# GPU Telemetry + Wolfram Engine Calculation Demo

Public MIT-licensed demo showing how to use **Wolfram Engine** as the local calculation engine behind a native C++/Qt dashboard.

The intended demo story is:

1. Qt samples GPU-like telemetry.
2. The dashboard calls Wolfram Engine through `wolframscript`.
3. Wolfram Language code computes the GPU health report locally.
4. Qt renders the result as a color-coded engineering dashboard.

The point of the demo is **why not just implement the calculation as a C++ DLL or library?** C++ is excellent for product code and high-performance systems integration, but it does not provide Wolfram Engine's breadth of built-in mathematics: symbolic manipulation, equation solving, optimization, statistics, time-series tools, and fast experimentation with formulas in one language/runtime.

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
- `src/GpuTelemetryApp` - Qt 6 Widgets dashboard with Wolfram Engine self-test mode.
- `src/MathematicaBridge` - WolframScript bridge used by the Wolfram Engine calculation step.
- `notebooks/GpuHealthReference.wl` - readable Wolfram Language calculation algorithm.
- `notebooks/GpuHealthPackageExample.m` - traditional Wolfram package-file example with symbolic helper APIs.
- `notebooks/GpuHealthReferenceRunner.wls` - command-line runner that imports telemetry JSON and exports Wolfram Engine calculation JSON.
- `tests/TestVectors` - synthetic sample window and expected output.

## Wolfram Engine-first calculation flow

The dashboard defaults to `Wolfram Engine Calculation Engine`. This is the first/demo calculation path: the Qt application samples telemetry, writes it to a temporary JSON file, and asks Wolfram Engine through `wolframscript` to calculate the health analysis:

```powershell
wolframscript -script .\notebooks\GpuHealthReferenceRunner.wls input.json output.json
```

Install and activate Wolfram Engine, then make `wolframscript.exe` available on `PATH` before running the dashboard.

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
```

The telemetry provider is safe on machines without NVML/NVIDIA hardware. It exposes a deterministic demo GPU and generated telemetry so the Wolfram Engine dashboard can be demonstrated anywhere. `--self-test` is kept as an alias for the Wolfram Engine self-test.

## Notes

- The current telemetry provider uses deterministic synthetic samples so the demo works on machines without NVIDIA/NVML.
- `notebooks/GpuHealthReference.wl` is the Wolfram Language calculation source.
- `notebooks/GpuHealthPackageExample.m` shows the traditional `.m` package format for reusable symbolic helpers such as score sensitivities and threshold solving.
- `notebooks/GpuHealthReferenceRunner.wls` is the WolframScript command-line bridge used by the Qt app.
- `docs/presentation/wolfram-engine-gpu-demo-presentation.html` is a customer-facing presentation explaining the download/install/licensing/demo flow.
