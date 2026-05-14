#pragma once

#include "GpuTelemetryTypes.h"

#include <string>
#include <vector>

std::string SamplesToJson(const std::vector<GpuTelemetrySample>& samples);
bool ParseSamplesJson(const std::string& json, std::vector<GpuTelemetrySample>& samples, std::string& error);
std::string AnalysisResultToJson(const AnalysisResult& result);
