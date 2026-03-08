# IFF Installer for Windows
$ErrorActionPreference = "Stop"

$installDir = "$env:LOCALAPPDATA\iff"
$iffExe     = "$installDir\iff.exe"
$baseUrl    = "https://github.com/Flaviuz1/IFF/releases/latest/download"

Write-Host "Downloading IFF compiler..."

# Create install directory
if (-not (Test-Path $installDir)) {
    New-Item -ItemType Directory -Path $installDir | Out-Null
}

# Download binary and required DLLs
Invoke-WebRequest -Uri "$baseUrl/iff.exe"                -OutFile "$installDir\iff.exe"
Invoke-WebRequest -Uri "$baseUrl/libgcc_s_seh-1.dll"     -OutFile "$installDir\libgcc_s_seh-1.dll"
Invoke-WebRequest -Uri "$baseUrl/libstdc++-6.dll"         -OutFile "$installDir\libstdc++-6.dll"

# Add to PATH if not already there
$currentPath = [Environment]::GetEnvironmentVariable("PATH", "User")
if ($currentPath -notlike "*$installDir*") {
    [Environment]::SetEnvironmentVariable("PATH", "$currentPath;$installDir", "User")
    Write-Host "Added $installDir to PATH."
    Write-Host "Restart your terminal for PATH changes to take effect."
} else {
    Write-Host "Already in PATH."
}

Write-Host "Done! Run 'iff yourfile.iff' to get started."