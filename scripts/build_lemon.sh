#!/usr/bin/env bash
# Build LEMON 1.3.1 static (-fPIC), no solver backends. Output: $MOGLI_CACHE/lemon-1.3.1
set -euo pipefail
. "$(dirname "$0")/common.sh"
LEMON_VERSION=1.3.1
LEMON_SHA256=71b7c725f4c0b4a8ccb92eb87b208701586cf7a96156ebd821ca3ed855bad3c8
PREFIX=${LEMON_PREFIX:-$MOGLI_CACHE/lemon-$LEMON_VERSION}
[ -f "$PREFIX/lib/libemon.a" ] && { echo "already built: $PREFIX"; exit 0; }
fetch_verified "https://lemon.cs.elte.hu/pub/sources/lemon-$LEMON_VERSION.tar.gz" lemon-$LEMON_VERSION.tar.gz $LEMON_SHA256
SRC=$(mktemp -d "$MOGLI_CACHE/lemon-src.XXXXXX"); trap 'rm -rf "$SRC"' EXIT
tar -xzf "$MOGLI_CACHE/dl/lemon-$LEMON_VERSION.tar.gz" -C "$SRC" --strip-components=1
cmake -S "$SRC" -B "$SRC/b" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_C_FLAGS=-fPIC -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DLEMON_ENABLE_GLPK=NO -DLEMON_ENABLE_COIN=NO -DLEMON_ENABLE_ILOG=NO -DLEMON_ENABLE_SOPLEX=NO \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build "$SRC/b" -j"$(nproc)"
cmake --install "$SRC/b"
