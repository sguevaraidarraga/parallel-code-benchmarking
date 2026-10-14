#!/usr/bin/env bash
# Compatibility entry point: checks only. The team runs the 30 repetitions.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
if [[ $# -eq 0 ]]; then
    set -- check
fi
exec python3 run_tests.py "$@"
