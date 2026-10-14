#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
if [[ $# -eq 0 ]]; then
    set -- full
fi
exec python3 run_tests.py "$@"
