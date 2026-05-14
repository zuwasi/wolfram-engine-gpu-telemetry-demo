#include "GpuHealthAnalyzer.h"
#include "JsonTelemetry.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

double Clamp(double value, double minValue, double maxValue) { return std::max(minValue, std::min(maxValue, value)); }

double Mean(const std::vector<double>& values)
{
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

double Variance(const std::vector<double>& values)
{
    if (values.size() < 2) return 0.0;
    const double mean = Mean(values);
    double sum = 0.0;
    for (double v : values) sum += (v - mean) * (v - mean);
    return sum / static_cast<double>(values.size() - 1);
}

double StandardDeviation(const std::vector<double>& values) { return std::sqrt(Variance(values)); }

double Median(std::vector<double> values)
{
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const auto n = values.size();
    return (n % 2 == 0) ? (values[n / 2 - 1] + values[n / 2]) / 2.0 : values[n / 2];
}

double MedianAbsoluteDeviation(const std::vector<double>& values)
{
    const double med = Median(values);
    std::vector<double> deviations;
    deviations.reserve(values.size());
    for (double v : values) deviations.push_back(std::abs(v - med));
    return Median(deviations);
}

double RobustZScore(double value, double median, double mad)
{
    if (mad <= 1.0e-9) return 0.0;
    return 0.6745 * (value - median) / mad;
}

double PearsonCorrelation(const std::vector<double>& x, const std::vector<double>& y)
{
    if (x.size() != y.size() || x.size() < 2) return 0.0;
    const double mx = Mean(x);
    const double my = Mean(y);
    double num = 0.0, dx = 0.0, dy = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        const double ax = x[i] - mx;
        const double ay = y[i] - my;
        num += ax * ay;
        dx += ax * ax;
        dy += ay * ay;
    }
    const double den = std::sqrt(dx * dy);
    return den <= 1.0e-9 ? 0.0 : Clamp(num / den, -1.0, 1.0);
}

std::vector<double> ExponentialSmooth(const std::vector<double>& values, double alpha)
{
    std::vector<double> out;
    if (values.empty()) return out;
    out.reserve(values.size());
    out.push_back(values.front());
    for (std::size_t i = 1; i < values.size(); ++i) out.push_back(alpha * values[i] + (1.0 - alpha) * out.back());
    return out;
}

double LinearRegressionSlope(const std::vector<double>& x, const std::vector<double>& y)
{
    if (x.size() != y.size() || x.size() < 2) return 0.0;
    const double mx = Mean(x);
    const double my = Mean(y);
    double num = 0.0, den = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        num += (x[i] - mx) * (y[i] - my);
        den += (x[i] - mx) * (x[i] - mx);
    }
    return den <= 1.0e-9 ? 0.0 : num / den;
}

AnalysisResult GpuHealthAnalyzer::Analyze(const std::vector<GpuTelemetrySample>& inputSamples) const
{
    std::vector<GpuTelemetrySample> samples = inputSamples;
    std::sort(samples.begin(), samples.end(), [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; });

    std::vector<double> t, temperature, power, powerLimit, util, memPct, memUsed, throttle;
    const auto t0 = samples.empty() ? std::chrono::system_clock::time_point{} : samples.front().timestamp;
    for (const auto& s : samples) {
        t.push_back(std::chrono::duration<double>(s.timestamp - t0).count());
        temperature.push_back(s.available.temperatureCelsius ? s.temperatureCelsius : 55.0);
        power.push_back(s.available.powerWatts ? s.powerWatts : 80.0);
        powerLimit.push_back(s.available.powerLimitWatts && s.powerLimitWatts > 1.0 ? s.powerLimitWatts : 140.0);
        util.push_back(s.available.gpuUtilizationPercent ? s.gpuUtilizationPercent : 40.0);
        double memory = s.available.memoryUtilizationPercent ? s.memoryUtilizationPercent : 35.0;
        if (s.available.memoryTotalBytes && s.available.memoryUsedBytes && s.memoryTotalBytes > 0) {
            memory = 100.0 * static_cast<double>(s.memoryUsedBytes) / static_cast<double>(s.memoryTotalBytes);
        }
        memPct.push_back(Clamp(memory, 0.0, 100.0));
        memUsed.push_back(static_cast<double>(s.memoryUsedBytes));
        throttle.push_back(s.throttleReasons != 0 ? 1.0 : 0.0);
    }

    const auto smoothTemp = ExponentialSmooth(temperature, 0.35);
    const auto smoothPower = ExponentialSmooth(power, 0.35);
    const auto smoothUtil = ExponentialSmooth(util, 0.35);
    const auto smoothMem = ExponentialSmooth(memPct, 0.35);

    const double tempNow = smoothTemp.empty() ? 55.0 : smoothTemp.back();
    const double slope = LinearRegressionSlope(t, smoothTemp);
    const double predictedTemp = tempNow + 60.0 * slope;
    const double powerMean = std::max(Mean(smoothPower), 1.0);
    const double cvPower = StandardDeviation(smoothPower) / powerMean;
    std::vector<double> powerDiff;
    for (std::size_t i = 1; i < smoothPower.size(); ++i) powerDiff.push_back(std::abs(smoothPower[i] - smoothPower[i - 1]) / powerMean);
    std::vector<double> ratios;
    for (std::size_t i = 0; i < power.size(); ++i) ratios.push_back(power[i] / std::max(powerLimit[i], 1.0));
    const double powerRatio = Clamp(Mean(ratios), 0.0, 1.5);
    const double powerInstability = Clamp(0.45 * cvPower + 0.35 * Mean(powerDiff) + 0.20 * Clamp((powerRatio - 0.75) / 0.25, 0.0, 1.0), 0.0, 1.0);

    const double utilVarianceRisk = Clamp(Variance(smoothUtil) / 900.0, 0.0, 1.0);
    const double utilPowerCorr = std::abs(PearsonCorrelation(smoothUtil, smoothPower));
    const double utilTempCorr = std::abs(PearsonCorrelation(smoothUtil, smoothTemp));
    const double utilizationStability = Clamp(1.0 - 0.55 * utilVarianceRisk + 0.30 * utilPowerCorr + 0.15 * (1.0 - utilTempCorr), 0.0, 1.0);

    std::vector<double> memDiff;
    for (std::size_t i = 1; i < smoothMem.size(); ++i) memDiff.push_back(std::abs(smoothMem[i] - smoothMem[i - 1]) / 100.0);
    const double memoryPressure = Clamp(0.70 * (Mean(smoothMem) / 100.0) + 0.30 * Mean(memDiff), 0.0, 1.0);

    const double tempMedian = Median(temperature), tempMad = MedianAbsoluteDeviation(temperature);
    const double powerMedian = Median(power), powerMad = MedianAbsoluteDeviation(power);
    const double utilMedian = Median(util), utilMad = MedianAbsoluteDeviation(util);
    int anomalyCount = 0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (std::abs(RobustZScore(temperature[i], tempMedian, tempMad)) > 3.5 ||
            std::abs(RobustZScore(power[i], powerMedian, powerMad)) > 3.5 ||
            std::abs(RobustZScore(util[i], utilMedian, utilMad)) > 3.5) ++anomalyCount;
    }
    const double anomalyRisk = Clamp(static_cast<double>(anomalyCount) / std::max<std::size_t>(samples.size(), 1), 0.0, 1.0);
    const double thermalRisk = Clamp(0.55 * ((tempNow - 70.0) / 18.0) + 0.30 * ((predictedTemp - 78.0) / 17.0) + 0.15 * Clamp(slope / 0.25, 0.0, 1.0), 0.0, 1.0);
    const double throttleRisk = Clamp(0.45 * Mean(throttle) + 0.30 * thermalRisk + 0.25 * Clamp((powerRatio - 0.90) / 0.10, 0.0, 1.0), 0.0, 1.0);
    const double health = Clamp(100.0 * (1.0 - 0.30 * thermalRisk - 0.20 * powerInstability - 0.15 * memoryPressure - 0.15 * anomalyRisk - 0.20 * throttleRisk), 0.0, 100.0);

    std::ostringstream diagnosis;
    if (health >= 90.0) diagnosis << "Excellent GPU condition";
    else if (health >= 75.0) diagnosis << "GPU is stable";
    else diagnosis << "GPU requires attention";
    if (thermalRisk > 0.45) diagnosis << "; watch thermal trend";
    if (powerInstability > 0.35) diagnosis << "; power instability detected";
    if (memoryPressure > 0.70) diagnosis << "; memory pressure detected";
    if (throttleRisk > 0.40) diagnosis << "; throttle risk detected";
    diagnosis << ".";

    AnalysisResult result;
    result.healthScore = health;
    result.thermalRisk = thermalRisk;
    result.powerInstability = powerInstability;
    result.memoryPressure = memoryPressure;
    result.utilizationStability = utilizationStability;
    result.predictedTemperature60s = predictedTemp;
    result.anomalyCount = anomalyCount;
    result.diagnosis = diagnosis.str();
    result.rawJson = AnalysisResultToJson(result);
    return result;
}
