#!/usr/bin/env bash
# Build Boost.Python (STATIC, -fPIC) for a given CPython. Output: $MOGLI_CACHE/boost-<ver>-py<pyver>
# usage: scripts/build_boost_python.sh [python-exe]   (default /usr/bin/python3.13)
set -euo pipefail
. "$(dirname "$0")/common.sh"
BOOST_VERSION=1.86.0
BOOST_SHA256=1bed88e40401b2cb7a1f76d4bab499e352fa4d0c5f31c0dbae64e24d34d7513b
PY=${1:-/usr/bin/python3.13}
PYVER=$("$PY" -c 'import sys;print("%d.%d"%sys.version_info[:2])')
PREFIX=${BOOST_PREFIX:-$MOGLI_CACHE/boost-$BOOST_VERSION-py$PYVER}
if ls "$PREFIX"/lib/libboost_python*.a >/dev/null 2>&1; then echo "already built: $PREFIX"; exit 0; fi
fetch_verified "https://archives.boost.io/release/$BOOST_VERSION/source/boost_${BOOST_VERSION//./_}.tar.bz2" \
  "boost_${BOOST_VERSION//./_}.tar.bz2" $BOOST_SHA256
SRC=$(mktemp -d "$MOGLI_CACHE/boost-src.XXXXXX"); trap 'rm -rf "$SRC"' EXIT
tar -xjf "$MOGLI_CACHE/dl/boost_${BOOST_VERSION//./_}.tar.bz2" -C "$SRC" --strip-components=1
cd "$SRC"
PYINC=$("$PY" -c 'import sysconfig;print(sysconfig.get_paths()["include"])')
./bootstrap.sh --with-libraries=python --with-python="$PY" --prefix="$PREFIX"
echo "using python : $PYVER : $PY : $PYINC ;" > user-config.jam
# gcc 11 lacks <stdfloat> (needed by 1.86 headers on newer -std): define BOOST_NO_CXX23_HDR_STDFLOAT
./b2 -j"$(nproc)" --user-config=user-config.jam --with-python python="$PYVER" \
  link=static runtime-link=shared threading=multi variant=release \
  cxxflags="-fPIC -DBOOST_NO_CXX23_HDR_STDFLOAT" install
echo "installed $PREFIX"; ls "$PREFIX/lib"
