# Implementation Notes

The first runnable implementation focuses on the demonstrable end-to-end path:

1. Qt 6 dashboard launches.
2. Sampling is responsive and safe on machines without NVIDIA/NVML.
3. Samples are serialized to JSON.
4. Wolfram Engine calculates the first-step analysis through `wolframscript`.
5. `GpuAnalysisDll.dll` is loaded at runtime for the later deployment path.
6. `--wolfram-engine-self-test` verifies Wolfram Engine calculation and `--self-test` verifies DLL loading without Wolfram Engine.

The telemetry provider currently uses deterministic synthetic telemetry as the no-crash fallback. A production NVML provider can replace it behind the existing `GpuTelemetryProvider` interface.
