#!/usr/bin/env bash
set -euo pipefail
test_bin="$(mktemp /tmp/m5-launcher-shortcut-test.XXXXXX)"
trap 'rm -f "$test_bin"' EXIT
c++ -std=c++17 -Wall -Wextra -Werror "$(dirname "$0")/launcher_shortcut_gesture_test.cpp" -o "$test_bin"
"$test_bin"
