# Captures the Direct3D shaders the plugin UI uses into GP\shaders\glint_d3d_shaders.bin,
# which the Windows builds embed (GP\config\GP-win.props -> resources\glint_d3d_shaders.rc).
# With it, the UI opens without compiling shaders, even on a user's first launch.
#
# Run again after updating Skia or changing what the UI draws: shaders the pack
# doesn't hold still work, they are just compiled (and disk-cached) at first use.
#
# Steps: build the standalone app, run it with capture on, then rebuild so it
# embeds the pack. While it runs, use the UI (hover, scroll, open every view):
# hover states and other views use their own shaders. Close the window when done.
# Shaders already in the pack are kept, so capturing again only adds to it.

param([string]$Configuration = 'Release')

$ErrorActionPreference = 'Stop'
$gp      = Split-Path -Parent $PSScriptRoot
$root    = Split-Path -Parent $gp
$pack    = Join-Path $gp 'shaders\glint_d3d_shaders.bin'
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe'
$bundle  = Join-Path $gp 'config\glint_bundle_shallow.props'

function Build-App {
  & $msbuild (Join-Path $gp 'GP.sln') /t:GP-app "/p:Configuration=$Configuration" /p:Platform=x64 "/p:ForceImportBeforeCppTargets=$bundle" /m /v:minimal /nologo
  if ($LASTEXITCODE -ne 0) { throw "build $Configuration failed" }
}

Push-Location $root
try {
  node third_party\glint\scripts\bundler\glint_bundler.mjs --input glint_user_code\web --output GP\glint_bundle --mode shallow
  if ($LASTEXITCODE -ne 0) { throw 'glint bundler failed' }
  Build-App

  New-Item -ItemType Directory -Force (Split-Path $pack) | Out-Null
  $env:GLINT_D3D_SHADER_CAPTURE = $pack
  # A private, empty disk cache: every shader goes through the capture.
  $env:GLINT_D3D_SHADER_CACHE_DIR = Join-Path ([IO.Path]::GetTempPath()) ("glint_shader_capture_" + [guid]::NewGuid().ToString('N'))
  try {
    Write-Host 'Use the plugin UI (hover, scroll, every view), then close the window...'
    Start-Process (Join-Path $gp "build-win\app\x64\$Configuration\GlintPlug.exe") -Wait
  }
  finally {
    Remove-Item Env:GLINT_D3D_SHADER_CAPTURE
    if (Test-Path -LiteralPath $env:GLINT_D3D_SHADER_CACHE_DIR) { Remove-Item -LiteralPath $env:GLINT_D3D_SHADER_CACHE_DIR -Recurse -Force }
    Remove-Item Env:GLINT_D3D_SHADER_CACHE_DIR
  }

  $bytes = [IO.File]::ReadAllBytes($pack)
  Write-Host ("Captured {0} shaders ({1:N0} KB) into {2}" -f [BitConverter]::ToUInt32($bytes, 8), ($bytes.Length / 1KB), $pack)

  # Rebuild so the binary embeds the (possibly new) pack.
  Build-App
}
finally {
  Pop-Location
}
