add_library(GpuTelemetryCore STATIC
    GpuHealthAnalyzer.cpp
    GpuTelemetryProvider.cpp
    JsonTelemetry.cpp
)

target_include_directories(GpuTelemetryCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(GpuTelemetryCore PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
