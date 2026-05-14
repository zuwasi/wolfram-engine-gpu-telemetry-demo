#include "JsonTelemetry.h"

#include <chrono>
#include <cctype>
#include <iomanip>
#include <regex>
#include <sstream>

namespace
{
std::int64_t ToUnixMs(std::chrono::system_clock::time_point tp)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}

std::chrono::system_clock::time_point FromUnixMs(std::int64_t ms)
{
    return std::chrono::system_clock::time_point{std::chrono::milliseconds(ms)};
}

std::string EscapeJson(const std::string& s)
{
    std::ostringstream out;
    for (char c : s) {
        if (c == '\\' || c == '"') out << '\\' << c;
        else if (c == '\n') out << "\\n";
        else out << c;
    }
    return out.str();
}

bool NumberField(const std::string& object, const std::string& name, double& value)
{
    const std::regex re("\\\"" + name + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)");
    std::smatch match;
    if (!std::regex_search(object, match, re)) return false;
    value = std::stod(match[1].str());
    return true;
}

bool UIntField(const std::string& object, const std::string& name, std::uint64_t& value)
{
    double v = 0.0;
    if (!NumberField(object, name, v)) return false;
    value = static_cast<std::uint64_t>(v);
    return true;
}

std::vector<std::string> ExtractObjects(const std::string& json)
{
    std::vector<std::string> objects;
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    std::size_t start = std::string::npos;
    for (std::size_t i = 0; i < json.size(); ++i) {
        const char c = json[i];
        if (inString) {
            escaped = (!escaped && c == '\\');
            if (!escaped && c == '"') inString = false;
            if (c != '\\') escaped = false;
            continue;
        }
        if (c == '"') inString = true;
        else if (c == '{') {
            if (depth == 0) start = i;
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0 && start != std::string::npos) objects.push_back(json.substr(start, i - start + 1));
        }
    }
    return objects;
}
}

std::string SamplesToJson(const std::vector<GpuTelemetrySample>& samples)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << "[";
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const auto& s = samples[i];
        if (i) out << ",";
        out << "{\"timestampUnixMs\":" << ToUnixMs(s.timestamp)
            << ",\"gpuName\":\"" << EscapeJson(s.gpuName) << "\""
            << ",\"temperatureCelsius\":" << s.temperatureCelsius
            << ",\"fanPercent\":" << s.fanPercent
            << ",\"powerWatts\":" << s.powerWatts
            << ",\"powerLimitWatts\":" << s.powerLimitWatts
            << ",\"gpuUtilizationPercent\":" << s.gpuUtilizationPercent
            << ",\"memoryUtilizationPercent\":" << s.memoryUtilizationPercent
            << ",\"memoryTotalBytes\":" << s.memoryTotalBytes
            << ",\"memoryUsedBytes\":" << s.memoryUsedBytes
            << ",\"graphicsClockMHz\":" << s.graphicsClockMHz
            << ",\"memoryClockMHz\":" << s.memoryClockMHz
            << ",\"throttleReasons\":" << s.throttleReasons << "}";
    }
    out << "]";
    return out.str();
}

bool ParseSamplesJson(const std::string& json, std::vector<GpuTelemetrySample>& samples, std::string& error)
{
    samples.clear();
    for (const auto& object : ExtractObjects(json)) {
        double ts = 0.0;
        GpuTelemetrySample s;
        if (!NumberField(object, "timestampUnixMs", ts)) continue;
        s.timestamp = FromUnixMs(static_cast<std::int64_t>(ts));
        s.available.temperatureCelsius = NumberField(object, "temperatureCelsius", s.temperatureCelsius);
        s.available.fanPercent = NumberField(object, "fanPercent", s.fanPercent);
        s.available.powerWatts = NumberField(object, "powerWatts", s.powerWatts);
        s.available.powerLimitWatts = NumberField(object, "powerLimitWatts", s.powerLimitWatts);
        s.available.gpuUtilizationPercent = NumberField(object, "gpuUtilizationPercent", s.gpuUtilizationPercent);
        s.available.memoryUtilizationPercent = NumberField(object, "memoryUtilizationPercent", s.memoryUtilizationPercent);
        s.available.memoryTotalBytes = UIntField(object, "memoryTotalBytes", s.memoryTotalBytes);
        s.available.memoryUsedBytes = UIntField(object, "memoryUsedBytes", s.memoryUsedBytes);
        s.available.graphicsClockMHz = NumberField(object, "graphicsClockMHz", s.graphicsClockMHz);
        s.available.memoryClockMHz = NumberField(object, "memoryClockMHz", s.memoryClockMHz);
        s.available.throttleReasons = UIntField(object, "throttleReasons", s.throttleReasons);
        samples.push_back(s);
    }
    if (samples.empty()) {
        error = "JSON did not contain telemetry samples";
        return false;
    }
    return true;
}

std::string AnalysisResultToJson(const AnalysisResult& r)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(3)
        << "{\"healthScore\":" << r.healthScore
        << ",\"thermalRisk\":" << r.thermalRisk
        << ",\"powerInstability\":" << r.powerInstability
        << ",\"memoryPressure\":" << r.memoryPressure
        << ",\"utilizationStability\":" << r.utilizationStability
        << ",\"predictedTemperature60s\":" << r.predictedTemperature60s
        << ",\"anomalyCount\":" << r.anomalyCount
        << ",\"diagnosis\":\"" << EscapeJson(r.diagnosis) << "\"}";
    return out.str();
}
