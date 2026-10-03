#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
command -v python3 >/dev/null 2>&1 || { echo 'python3 is required for exact bundle verification' >&2; exit 1; }
exec python3 "$root/install_linux.py"
