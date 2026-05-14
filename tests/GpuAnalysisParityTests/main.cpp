#include "GpuAnalysisDll.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

namespace
{
std::string ReadFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

double Field(const std::string& json, const std::string& name)
{
    const std::regex re("\\\"" + name + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch m;
    if (!std::regex_search(json, m, re)) return std::nan("");
    return std::stod(m[1].str());
}
}

int main()
{
    const std::string inputPath = std::string(TEST_VECTOR_DIR) + "/sample_gpu_window_001.json";
    const std::string expectedPath = std::string(TEST_VECTOR_DIR) + "/sample_gpu_window_001_expected_mathematica.json";
    const std::string input = ReadFile(inputPath);
    const std::string expected = ReadFile(expectedPath);
    char output[4096] = {};
    const int rc = AnalyzeGpuTelemetryJson(input.c_str(), output, sizeof(output));
    if (rc != 0) {
        std::cerr << "AnalyzeGpuTelemetryJson failed with code " << rc << "\n";
        return 1;
    }
    const std::string actual = output;
    const struct { const char* name; double tolerance; } checks[] = {
        {"healthScore", 2.0}, {"thermalRisk", 0.03}, {"powerInstability", 0.03},
        {"memoryPressure", 0.03}, {"utilizationStability", 0.03}, {"predictedTemperature60s", 1.0},
        {"anomalyCount", 0.0}
    };
    for (const auto& check : checks) {
        const double a = Field(actual, check.name);
        const double e = Field(expected, check.name);
        if (!std::isfinite(a) || !std::isfinite(e) || std::abs(a - e) > check.tolerance) {
            std::cerr << "Parity failed for " << check.name << ": actual=" << a << " expected=" << e << "\n";
            std::cerr << "Actual JSON: " << actual << "\n";
            return 1;
        }
    }
    std::cout << "Parity OK: " << actual << "\n";
    return 0;
}
