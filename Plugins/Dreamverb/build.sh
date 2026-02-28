#!/bin/bash
set -euo pipefail

echo ""
echo "======================================"
echo "  Dreamverb — Sound Capsule Build"
echo "======================================"
echo ""

# 1. Xcode Command Line Tools
if ! xcode-select -p &>/dev/null; then
  echo "Installing Xcode Command Line Tools..."
  xcode-select --install
  echo "Run this script again once Xcode CLT finishes installing."
  exit 0
fi
echo "OK: Xcode CLT found"

# 2. Homebrew
if ! command -v brew &>/dev/null; then
  echo "Installing Homebrew..."
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  eval "$(/opt/homebrew/bin/brew shellenv)" 2>/dev/null || true
  eval "$(/usr/local/bin/brew shellenv)" 2>/dev/null || true
fi
echo "OK: Homebrew found"

# 3. cmake + git
command -v cmake &>/dev/null || brew install cmake
command -v git   &>/dev/null || brew install git
echo "OK: cmake and git ready"

# 4. Configure — downloads JUCE automatically (first run ~200MB)
echo ""
echo "Configuring... first run downloads JUCE, grab a coffee."
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 5. Compile
echo ""
echo "Building Dreamverb..."
CORES=$(sysctl -n hw.logicalcpu)
cmake --build build --config Release --parallel $CORES

# 6. Install
VST3_SRC=$(find build -name "Dreamverb.vst3" -type d 2>/dev/null | head -1)
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$VST3_DIR"

if [ -n "$VST3_SRC" ]; then
  rm -rf "$VST3_DIR/Dreamverb.vst3"
  cp -r "$VST3_SRC" "$VST3_DIR/"
  echo ""
  echo "======================================"
  echo "  DONE: Dreamverb.vst3 installed!"
  echo "======================================"
  echo ""
  echo "Open Ableton > Preferences > Plug-Ins > Rescan"
  echo "Find: Sound Capsule > Dreamverb"
  echo ""
else
  echo "ERROR: .vst3 not found after build. Check the build/ folder."
  exit 1
fi