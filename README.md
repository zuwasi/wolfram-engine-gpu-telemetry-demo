# GPU Telemetry + Wolfram Engine Algorithm Translation Demo

Public MIT-licensed demo showing how to use **Wolfram Engine** as a first-step calculation engine from a native C++/Qt dashboard, then compare that flow with a translated native C++ DLL implementation.

The intended demo story is:

1. Qt samples GPU-like telemetry.
2. The first calculation path calls Wolfram Engine through `wolframscript`.
3. Wolfram Language code computes the GPU health report.
4. The same algorithm is available as a native C++ DLL for later deployment without Wolfram Engine.

This project is a runnable C++/Qt implementation of the specification bundle in `../amp_gpu_mathematica_demo`.

## License

MIT. See [LICENSE](LICENSE).

## Requirements

- Windows 11
- CMake 3.24+
- Qt 6 Widgets with MinGW toolchain, tested with `C:\Qt\6.8.3\mingw_64`
- Wolfram Engine 14.3+ or compatible Wolfram runtime providing `wolframscript.exe`
- Activated Wolfram Engine license

## Contents

- `src/GpuTelemetryCore` - telemetry types, synthetic-safe telemetry provider, JSON helpers, native analyzer.
- `src/GpuTelemetryApp` - Qt 6 Widgets dashboard with `--self-test` mode.
- `src/GpuAnalysisDll` - native C ABI DLL: `AnalyzeGpuTelemetryJson`.
- `src/MathematicaBridge` - development-time WolframScript bridge used by the first Wolfram Engine calculation step.
- `notebooks/GpuHealthReference.wl` - readable Wolfram Language calculation algorithm.
- `notebooks/GpuHealthReferenceRunner.wls` - command-line runner that imports telemetry JSON and exports Wolfram Engine calculation JSON.
- `tests/GpuAnalysisParityTests` - C++ parity smoke test against saved expected JSON.
- `tests/TestVectors` - synthetic sample window and expected output.

## Wolfram Engine-first calculation flow

The dashboard defaults to `Wolfram Engine Calculation Engine`. This is the first/demo calculation path: the Qt application samples telemetry, writes it to a temporary JSON file, and asks Wolfram Engine through `wolframscript` to calculate the health analysis:

```powershell
wolframscript -script .\notebooks\GpuHealthReferenceRunner.wls input.json output.json
```

Install and activate Wolfram Engine, then make `wolframscript.exe` available on `PATH` before using that first-step engine. The `Native C++ DLL` option is the later deployment path and does not require Wolfram Engine.

Verify Wolfram Engine from PowerShell:

```powershell
wolframscript.exe -code '$Version'
```

## Build

```powershell
$env:PATH="C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.8.3\mingw_64\bin;$env:PATH"
cmake --preset mingw-qt-debug
cmake --build --preset mingw-qt-debug
```

## Run

```powershell
.\build\mingw-qt-debug\bin\GpuTelemetryDashboard.exe
.\build\mingw-qt-debug\bin\GpuTelemetryDashboard.exe --wolfram-engine-self-test
.\build\mingw-qt-debug\bin\GpuTelemetryDashboard.exe --mathematica-self-test
.\build\mingw-qt-debug\bin\GpuTelemetryDashboard.exe --self-test
.\build\mingw-qt-debug\bin\GpuAnalysisParityTests.exe
```

The telemetry provider is safe on machines without NVML/NVIDIA hardware. It exposes a deterministic demo GPU and generated telemetry so the dashboard and native DLL can be demonstrated anywhere. The native runtime does not call Wolfram Engine.

## Notes

- The current telemetry provider uses deterministic synthetic samples so the demo works on machines without NVIDIA/NVML.
- `notebooks/GpuHealthReference.wl` is the Wolfram Language calculation source.
- `notebooks/GpuHealthReferenceRunner.wls` is the WolframScript command-line bridge used by the Qt app.
- `src/GpuAnalysisDll` demonstrates the later native C++ deployment path.
