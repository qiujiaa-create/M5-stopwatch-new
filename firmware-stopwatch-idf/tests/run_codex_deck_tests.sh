#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$root/build-v2/host-deck"
cmake -S "$root/tests/codex_deck_host" -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$build_dir" -j 6
"$build_dir/codex_deck_host" "$root/build-v2/ui-preview"
if command -v sips >/dev/null 2>&1; then
    for preview in "$root/build-v2/ui-preview/"*.ppm; do
        sips -s format png "$preview" --out "${preview%.ppm}.png" >/dev/null
    done
fi
