@echo off
REM - CALL "$(SolutionDir)scripts\prebuild-win.bat" "$(TargetExt)" "$(BINARY_NAME)" "$(Platform)" "$(TargetPath)" "$(OutDir)" "$(Configuration)"
REM set FORMAT=%1
REM set NAME=%2
REM set PLATFORM=%3
REM set BUILT_BINARY=%4
REM set CONFIGURATION=%6

REM ── Auto-increment patch version in config.h (Debug builds only) ────────────
REM   Release builds use the version exactly as committed — do not bump.
set CONFIGURATION=%~6
if /I "%CONFIGURATION%"=="Release" (
  echo [prebuild-win] Release build — skipping version bump.
  goto :eof
)

REM   %~dp0 is GP\scripts\, so repo root is two levels up.
pushd "%~dp0..\.."
where node >nul 2>nul
if errorlevel 1 (
  echo [prebuild-win] WARNING: node.exe not found on PATH; skipping build-number bump.
) else (
  node "scripts\bump_build_number.mjs"
  if errorlevel 1 echo [prebuild-win] WARNING: bump_build_number.mjs failed.
)
popd