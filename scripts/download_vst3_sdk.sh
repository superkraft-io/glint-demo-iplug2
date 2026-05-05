#!/usr/bin/env bash
set -eo pipefail

# 1st argument = tag name
# 2nd argument = "build-validator" to build the vst3-validator

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_DIR="$SCRIPT_DIR/../third_party/iPlug2OOS/iPlug2/Dependencies/IPlug/VST3_SDK"

TAG="master"
if [ "$1" != "" ]; then
  TAG=$1
fi

rm -f -r "$SDK_DIR"
git clone https://github.com/steinbergmedia/vst3sdk.git --branch $TAG --single-branch --depth=1 "$SDK_DIR"
cd "$SDK_DIR"
git submodule update --init pluginterfaces
git submodule update --init base
git submodule update --init public.sdk
git submodule update --init cmake
git submodule update --init vstgui4

if [ "$2" == "build-validator" ]; then
  mkdir VST3_BUILD
  cd VST3_BUILD
  if [ "$(uname)" == "Darwin" ]; then
    cmake .. -G Xcode
  else
    cmake ..
  fi
  cmake --build . --target validator -j --config=Release
  if [ -d "./bin/Release" ]; then
    mv ./bin/Release/validator ../validator
  else
    mv ./bin/validator ../validator
  fi
  cd ..
fi

rm -f -r VST3_BUILD
rm -f -r public.sdk/samples
rm -f -r vstgui4
rm -f -r .git*
rm -f -r */.git*
git checkout README.md
