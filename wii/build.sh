#!/bin/bash
# Build the .dol with devkitPPC. Run from anywhere: the cd below resolves to
# this script's own directory rather than a hardcoded path, which is what the
# WSL-era version had and what stopped working the moment the checkout moved.
set -euo pipefail

export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
export DEVKITPPC=${DEVKITPPC:-$DEVKITPRO/devkitPPC}
export PATH="$DEVKITPPC/bin:$PATH"

cd "$(dirname "$0")"
make clean
make
