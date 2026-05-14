#pragma once

#include "GpuTelemetryTypes.h"

#include <string>
#include <vector>

class GpuTelemetryProvider
{
public:
    std::vector<std::string> EnumerateGpus() const;
    GpuTelemetrySample ReadSample(int gpuIndex, int sampleIndex) const;
};
