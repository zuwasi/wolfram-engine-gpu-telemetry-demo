#pragma once

#include "GpuTelemetryTypes.h"

#include <vector>

double Clamp(double value, double minValue, double maxValue);
double Mean(const std::vector<double>& values);
double Variance(const std::vector<double>& values);
double StandardDeviation(const std::vector<double>& values);
double Median(std::vector<double> values);
double MedianAbsoluteDeviation(const std::vector<double>& values);
double RobustZScore(double value, double median, double mad);
double PearsonCorrelation(const std::vector<double>& x, const std::vector<double>& y);
std::vector<double> ExponentialSmooth(const std::vector<double>& values, double alpha);
double LinearRegressionSlope(const std::vector<double>& x, const std::vector<double>& y);

class GpuHealthAnalyzer
{
public:
    AnalysisResult Analyze(const std::vector<GpuTelemetrySample>& samples) const;
};
