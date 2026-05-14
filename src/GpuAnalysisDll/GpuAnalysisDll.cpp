#include "GpuAnalysisDll.h"

#include "GpuHealthAnalyzer.h"
#include "JsonTelemetry.h"

#include <cstring>
#include <string>
#include <vector>

int AnalyzeGpuTelemetryJson(const char* inputJson, char* outputJson, int outputJsonCapacity)
{
    try {
        if (!inputJson || !outputJson || outputJsonCapacity <= 0) return 1;
        std::vector<GpuTelemetrySample> samples;
        std::string error;
        if (!ParseSamplesJson(inputJson, samples, error)) return 2;
        if (samples.size() < 5) return 3;
        const auto result = GpuHealthAnalyzer{}.Analyze(samples);
        const std::string json = AnalysisResultToJson(result);
        if (static_cast<int>(json.size() + 1) > outputJsonCapacity) return 4;
        std::memcpy(outputJson, json.c_str(), json.size() + 1);
        return 0;
    } catch (...) {
        return 5;
    }
}
