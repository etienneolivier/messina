#!/bin/bash

# Exit on any error
set -e

# Directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
BUILD_DIR="${SCRIPT_DIR}/build"
UI_DIR="${SCRIPT_DIR}/ui"

echo "Building React UI..."
cd "${UI_DIR}"
npm install
npm run build
python3 inline_assets.py

echo "Building JUCE Plugin..."
cd "${SCRIPT_DIR}"
cmake -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" --config Release -j$(sysctl -n hw.ncpu)

echo "Done! Plugins built successfully in ${BUILD_DIR}/Messina_artefacts/"
