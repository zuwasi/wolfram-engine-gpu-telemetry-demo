#pragma once

#ifdef _WIN32
#ifdef GPU_ANALYSIS_DLL_EXPORTS
#define GPU_ANALYSIS_API extern "C" __declspec(dllexport)
#else
#define GPU_ANALYSIS_API extern "C" __declspec(dllimport)
#endif
#else
#define GPU_ANALYSIS_API extern "C"
#endif

GPU_ANALYSIS_API int AnalyzeGpuTelemetryJson(const char* inputJson, char* outputJson, int outputJsonCapacity);
