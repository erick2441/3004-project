#!/usr/bin/env bash
set -e

cd "$(dirname "$0")"
mkdir -p .build
cd .build
qmake ../DragonEndPOS.pro
make
exec ./DragonEndPOS
