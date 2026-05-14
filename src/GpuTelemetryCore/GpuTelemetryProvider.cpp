#include "GpuTelemetryProvider.h"

#include <chrono>
#include <cmath>

std::vector<std::string> GpuTelemetryProvider::EnumerateGpus() const
{
    return {"Demo GPU (NVML-safe synthetic telemetry)"};
}

GpuTelemetrySample GpuTelemetryProvider::ReadSample(int, int sampleIndex) const
{
    GpuTelemetrySample s;
    s.timestamp = std::chrono::system_clock::now();
    s.gpuName = "Demo GPU (NVML-safe synthetic telemetry)";
    s.temperatureCelsius = 62.0 + 0.30 * sampleIndex + 1.8 * std::sin(sampleIndex * 0.45);
    s.fanPercent = 41.0 + 0.20 * sampleIndex;
    s.powerWatts = 104.0 + 8.0 * std::sin(sampleIndex * 0.7);
    s.powerLimitWatts = 145.0;
    s.gpuUtilizationPercent = 72.0 + 12.0 * std::sin(sampleIndex * 0.55);
    s.memoryUtilizationPercent = 48.0 + 0.35 * sampleIndex;
    s.memoryTotalBytes = 16ull * 1024ull * 1024ull * 1024ull;
    s.memoryUsedBytes = static_cast<std::uint64_t>(s.memoryTotalBytes * (s.memoryUtilizationPercent / 100.0));
    s.graphicsClockMHz = 1815.0;
    s.memoryClockMHz = 7000.0;
    s.throttleReasons = sampleIndex > 24 ? 1 : 0;
    s.available = {true, true, true, true, true, true, true, true, true, true, true};
    return s;
}
