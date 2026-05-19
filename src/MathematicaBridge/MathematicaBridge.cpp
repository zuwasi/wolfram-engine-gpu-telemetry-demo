#include "MathematicaBridge.h"

#include "JsonTelemetry.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
std::string Quote(const std::filesystem::path& path)
{
    return "\"" + path.string() + "\"";
}

std::string EscapeJson(const std::string& text)
{
    std::ostringstream out;
    for (char c : text) {
        if (c == '\\' || c == '"') out << '\\' << c;
        else if (c == '\n') out << "\\n";
        else if (c == '\r') out << "\\r";
        else out << c;
    }
    return out.str();
}

std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

void WriteFile(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream out(path, std::ios::binary);
    out << text;
}

bool NumberField(const std::string& json, const std::string& name, double& value)
{
    const std::regex re("\\\"" + name + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)");
    std::smatch match;
    if (!std::regex_search(json, match, re)) return false;
    value = std::stod(match[1].str());
    return true;
}

bool IntField(const std::string& json, const std::string& name, int& value)
{
    double number = 0.0;
    if (!NumberField(json, name, number)) return false;
    value = static_cast<int>(number);
    return true;
}

AnalysisResult ErrorResult(const std::string& message)
{
    AnalysisResult result;
    result.diagnosis = message;
    result.rawJson = "{\"healthScore\":0,\"thermalRisk\":0,\"powerInstability\":0,\"memoryPressure\":0,\"utilizationStability\":0,\"predictedTemperature60s\":0,\"anomalyCount\":0,\"diagnosis\":\"" + EscapeJson(message) + "\"}";
    return result;
}

std::filesystem::path ExecutableDirectory()
{
#ifdef _WIN32
    char path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (length > 0 && length < MAX_PATH) return std::filesystem::path(path).parent_path();
#endif
    return std::filesystem::current_path();
}

std::filesystem::path FindWolframRunner(WolframCalculationMode mode)
{
    const auto exeDir = ExecutableDirectory();
    const char* runnerName = mode == WolframCalculationMode::PackageFile
        ? "GpuHealthPackageRunner.wls"
        : "GpuHealthReferenceRunner.wls";
    const std::filesystem::path candidates[] = {
        exeDir / "notebooks" / runnerName,
        exeDir.parent_path() / "notebooks" / runnerName,
        std::filesystem::path(GPU_DEMO_SOURCE_DIR) / "notebooks" / runnerName
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) return candidate;
    }

    return candidates[0];
}

int RunHiddenCommand(const std::string& command)
{
#ifdef _WIN32
    STARTUPINFOA startupInfo{};
    PROCESS_INFORMATION processInfo{};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_HIDE;

    std::string mutableCommand = command;
    if (!CreateProcessA(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo)) {
        return static_cast<int>(GetLastError());
    }

    constexpr DWORD timeoutMs = 120000;
    const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, timeoutMs);
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(processInfo.hProcess, WAIT_TIMEOUT);
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        return static_cast<int>(WAIT_TIMEOUT);
    }
    DWORD exitCode = 1;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return static_cast<int>(exitCode);
#else
    return std::system(command.c_str());
#endif
}
}

MathematicaAnalysisEngine::MathematicaAnalysisEngine(WolframCalculationMode mode)
    : mode_(mode)
{
}

AnalysisResult MathematicaAnalysisEngine::Analyze(const std::vector<GpuTelemetrySample>& samples)
{
    const auto runner = FindWolframRunner(mode_);
    if (!std::filesystem::exists(runner)) {
        return ErrorResult("Wolfram Engine runner not found: " + runner.string());
    }

    const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const auto tempDir = std::filesystem::temp_directory_path();
    const auto inputPath = tempDir / ("gpu_telemetry_mathematica_input_" + std::to_string(stamp) + ".json");
    const auto outputPath = tempDir / ("gpu_telemetry_mathematica_output_" + std::to_string(stamp) + ".json");

    WriteFile(inputPath, SamplesToJson(samples));
    const std::string command = "wolframscript -script " + Quote(runner) + " " + Quote(inputPath) + " " + Quote(outputPath);
    const int exitCode = RunHiddenCommand(command);
    std::error_code ignored;
    std::filesystem::remove(inputPath, ignored);
    if (exitCode != 0 || !std::filesystem::exists(outputPath)) {
        return ErrorResult("Wolfram Engine calculation failed. Ensure Wolfram Engine is activated and wolframscript is on PATH. Command: " + command);
    }

    const std::string json = ReadFile(outputPath);
    std::filesystem::remove(outputPath, ignored);
    AnalysisResult result;
    NumberField(json, "healthScore", result.healthScore);
    NumberField(json, "thermalRisk", result.thermalRisk);
    NumberField(json, "powerInstability", result.powerInstability);
    NumberField(json, "memoryPressure", result.memoryPressure);
    NumberField(json, "utilizationStability", result.utilizationStability);
    NumberField(json, "predictedTemperature60s", result.predictedTemperature60s);
    IntField(json, "anomalyCount", result.anomalyCount);
    result.diagnosis = "Wolfram Engine calculation completed.";
    result.rawJson = json;
    return result;
}
