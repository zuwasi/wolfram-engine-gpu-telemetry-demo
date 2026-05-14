#pragma once

#include "GpuTelemetryTypes.h"

class MathematicaAnalysisEngine final : public IAnalysisEngine
{
public:
    AnalysisResult Analyze(const std::vector<GpuTelemetrySample>& samples) override;
};

class MockAnalysisEngine final : public IAnalysisEngine
{
public:
    AnalysisResult Analyze(const std::vector<GpuTelemetrySample>& samples) override;
};
