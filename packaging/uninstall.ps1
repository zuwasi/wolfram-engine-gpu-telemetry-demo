param(
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\WolframEngineGpuTelemetryDemo"
)

$ErrorActionPreference = 'Stop'
$installDirFull = [System.IO.Path]::GetFullPath($InstallDir)

Get-Process -Name GpuTelemetryDashboard -ErrorAction SilentlyContinue | Stop-Process -Force

$startMenuDir = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Wolfram Engine GPU Telemetry Demo'
$desktopShortcut = Join-Path ([Environment]::GetFolderPath('Desktop')) 'Wolfram Engine GPU Telemetry Demo.lnk'

Remove-Item -Path $startMenuDir -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path $desktopShortcut -Force -ErrorAction SilentlyContinue
Remove-Item -Path $installDirFull -Recurse -Force -ErrorAction SilentlyContinue

Write-Host 'Uninstall complete.'
