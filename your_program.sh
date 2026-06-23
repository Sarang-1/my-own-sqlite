#!/bin/sh
set -e

(
  cd "$(dirname "$0")"

  if [ "$(uname -s)" != "Linux" ]; then
    cmake -B build -S . \
      -DCMAKE_C_COMPILER="D:/w64devkit/bin/gcc.exe" \
      -DCMAKE_CXX_COMPILER="D:/w64devkit/bin/g++.exe" \
      -G "MinGW Makefiles"
    
    cmake --build ./build
  fi
)

# Point this to "server" instead of "sqlite"
exec $(dirname "$0")/build/server "$@"