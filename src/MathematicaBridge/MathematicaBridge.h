#pragma once

#include "GpuTelemetryTypes.h"

enum class WolframCalculationMode
{
    ScriptRunner,
    PackageFile
};

class MathematicaAnalysisEngine final : public IAnalysisEngine
{
public:
    explicit MathematicaAnalysisEngine(WolframCalculationMode mode = WolframCalculationMode::ScriptRunner);
    AnalysisResult Analyze(const std::vector<GpuTelemetrySample>& samples) override;

private:
    WolframCalculationMode mode_;
};
