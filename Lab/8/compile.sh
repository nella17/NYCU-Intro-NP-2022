#!/bin/bash
set -x

CFLAGS="-static -O3 -I./zstd/lib zstd/lib/libzstd.a"
g++ src/server.cpp -o bin/server.exe $CFLAGS &
g++ src/client.cpp -o bin/client.exe $CFLAGS &
wait
