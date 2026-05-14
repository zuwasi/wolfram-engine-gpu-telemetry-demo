#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

struct GpuMetricAvailability
{
    bool temperatureCelsius = false;
    bool fanPercent = false;
    bool powerWatts = false;
    bool powerLimitWatts = false;
    bool gpuUtilizationPercent = false;
    bool memoryUtilizationPercent = false;
    bool memoryTotalBytes = false;
    bool memoryUsedBytes = false;
    bool graphicsClockMHz = false;
    bool memoryClockMHz = false;
    bool throttleReasons = false;
};

struct GpuTelemetrySample
{
    std::chrono::system_clock::time_point timestamp{};
    std::string gpuName;
    double temperatureCelsius = 0.0;
    double fanPercent = 0.0;
    double powerWatts = 0.0;
    double powerLimitWatts = 0.0;
    double gpuUtilizationPercent = 0.0;
    double memoryUtilizationPercent = 0.0;
    std::uint64_t memoryTotalBytes = 0;
    std::uint64_t memoryUsedBytes = 0;
    double graphicsClockMHz = 0.0;
    double memoryClockMHz = 0.0;
    std::uint64_t throttleReasons = 0;
    GpuMetricAvailability available;
};

struct AnalysisResult
{
    double healthScore = 0.0;
    double thermalRisk = 0.0;
    double powerInstability = 0.0;
    double memoryPressure = 0.0;
    double utilizationStability = 0.0;
    double predictedTemperature60s = 0.0;
    int anomalyCount = 0;
    std::string diagnosis;
    std::string rawJson;
};

class IAnalysisEngine
{
public:
    virtual ~IAnalysisEngine() = default;
    virtual AnalysisResult Analyze(const std::vector<GpuTelemetrySample>& samples) = 0;
};
