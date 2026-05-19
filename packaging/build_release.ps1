param(
    [string]$Configuration = 'Release',
    [string]$Preset = 'mingw-qt-release',
    [string]$Version = '0.1.3'
)

$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $projectDir 'build\mingw-qt-release'
$distRoot = Join-Path $projectDir 'dist'
$packageName = "WolframEngineGpuTelemetryDemo-$Version-win64"
$packageDir = Join-Path $distRoot $packageName
$zipPath = Join-Path $distRoot "$packageName.zip"

$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.8.3\mingw_64\bin;$env:PATH"

Set-Location $projectDir
cmake --preset $Preset
cmake --build --preset $Preset

Remove-Item -Path $packageDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null

$binDir = Join-Path $buildDir 'bin'
Copy-Item (Join-Path $binDir 'GpuTelemetryDashboard.exe') $packageDir -Force
Copy-Item (Join-Path $projectDir 'LICENSE') $packageDir -Force
Copy-Item (Join-Path $PSScriptRoot 'README_RUNTIME.txt') $packageDir -Force
Copy-Item (Join-Path $PSScriptRoot 'CUSTOMER_INSTALL_GUIDE.md') $packageDir -Force
Copy-Item (Join-Path $PSScriptRoot 'install.ps1') $packageDir -Force
Copy-Item (Join-Path $PSScriptRoot 'uninstall.ps1') $packageDir -Force
Copy-Item (Join-Path $projectDir 'notebooks') (Join-Path $packageDir 'notebooks') -Recurse -Force

windeployqt.exe --release --compiler-runtime --no-translations (Join-Path $packageDir 'GpuTelemetryDashboard.exe')

Push-Location $packageDir
$wolframCheck = & wolframscript.exe -code '$Version' 2>$null
if ($LASTEXITCODE -eq 0) {
    Write-Host "Wolfram Engine available for local verification: $wolframCheck"
    & .\GpuTelemetryDashboard.exe --wolfram-engine-self-test
} else {
    Write-Warning 'wolframscript.exe not available; skipping Wolfram Engine package self-test.'
}
Pop-Location

Remove-Item -Path $zipPath -Force -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $packageDir '*') -DestinationPath $zipPath -Force

Write-Host "Release package created: $zipPath"
