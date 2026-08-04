#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build-tests"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

c++ -std=c++23 -Wall -Wextra -Werror \
  "$ROOT/tests/selection_utils_test.cpp" \
  -o "$BUILD_DIR/selection_utils_test"

"$BUILD_DIR/selection_utils_test"
echo "Selection utility tests passed."
