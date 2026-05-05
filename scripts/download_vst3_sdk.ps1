# download_vst3_sdk.ps1
# 1st argument = tag name
# 2nd argument = "build-validator" to build the vst3-validator

param(
    [string]$Tag = "master",
    [string]$BuildValidator = ""
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$SdkDir = Join-Path $ScriptDir "..\third_party\iPlug2OOS\iPlug2\Dependencies\IPlug\VST3_SDK"
$SdkDir = [System.IO.Path]::GetFullPath($SdkDir)

if (Test-Path $SdkDir) {
    Remove-Item -Recurse -Force $SdkDir
}

git clone https://github.com/steinbergmedia/vst3sdk.git --branch $Tag --single-branch --depth=1 $SdkDir
Set-Location $SdkDir
git submodule update --init pluginterfaces
git submodule update --init base
git submodule update --init public.sdk
git submodule update --init cmake
git submodule update --init vstgui4

if ($BuildValidator -eq "build-validator") {
    New-Item -ItemType Directory -Name VST3_BUILD | Out-Null
    Set-Location VST3_BUILD
    cmake ..
    cmake --build . --target validator -j --config Release
    if (Test-Path ".\bin\Release\validator.exe") {
        Move-Item ".\bin\Release\validator.exe" "..\validator.exe"
    } elseif (Test-Path ".\bin\validator.exe") {
        Move-Item ".\bin\validator.exe" "..\validator.exe"
    }
    Set-Location ..
}

if (Test-Path "VST3_BUILD") { Remove-Item -Recurse -Force "VST3_BUILD" }
if (Test-Path "public.sdk\samples") { Remove-Item -Recurse -Force "public.sdk\samples" }
if (Test-Path "vstgui4") { Remove-Item -Recurse -Force "vstgui4" }
Get-ChildItem -Path . -Filter ".git*" -Force | Remove-Item -Recurse -Force
Get-ChildItem -Path . -Recurse -Filter ".git*" -Force | Remove-Item -Recurse -Force
git checkout README.md
