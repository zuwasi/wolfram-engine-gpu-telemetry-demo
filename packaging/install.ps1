param(
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\WolframEngineGpuTelemetryDemo"
)

$ErrorActionPreference = 'Stop'
$sourceDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$installDirFull = [System.IO.Path]::GetFullPath($InstallDir)

Write-Host "Installing Wolfram Engine GPU Telemetry Demo to $installDirFull"
New-Item -ItemType Directory -Force -Path $installDirFull | Out-Null

$exclude = @('install.ps1')
Get-ChildItem -Path $sourceDir -Force | Where-Object { $exclude -notcontains $_.Name } | ForEach-Object {
    $destination = Join-Path $installDirFull $_.Name
    if ($_.PSIsContainer) {
        Copy-Item -Path $_.FullName -Destination $destination -Recurse -Force
    } else {
        Copy-Item -Path $_.FullName -Destination $destination -Force
    }
}

$exePath = Join-Path $installDirFull 'GpuTelemetryDashboard.exe'
if (!(Test-Path $exePath)) {
    throw "GpuTelemetryDashboard.exe was not found after copy. Package is incomplete."
}

$shell = New-Object -ComObject WScript.Shell
$startMenuDir = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Wolfram Engine GPU Telemetry Demo'
New-Item -ItemType Directory -Force -Path $startMenuDir | Out-Null

$startShortcut = $shell.CreateShortcut((Join-Path $startMenuDir 'Wolfram Engine GPU Telemetry Demo.lnk'))
$startShortcut.TargetPath = $exePath
$startShortcut.WorkingDirectory = $installDirFull
$startShortcut.Description = 'Wolfram Engine GPU Telemetry Demo'
$startShortcut.Save()

$desktopShortcut = $shell.CreateShortcut((Join-Path ([Environment]::GetFolderPath('Desktop')) 'Wolfram Engine GPU Telemetry Demo.lnk'))
$desktopShortcut.TargetPath = $exePath
$desktopShortcut.WorkingDirectory = $installDirFull
$desktopShortcut.Description = 'Wolfram Engine GPU Telemetry Demo'
$desktopShortcut.Save()

Write-Host 'Install complete.'
Write-Host 'Before running, verify Wolfram Engine with: wolframscript.exe -code ''$Version'''
