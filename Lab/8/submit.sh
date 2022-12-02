#!/bin/bash
set -ex
CFLAGS="-static -O3 -I./zstd/lib zstd/lib/libzstd.a"
g++ src/server.cpp -o bin/server.static.exe $CFLAGS &
g++ src/client.cpp -o bin/client.static.exe $CFLAGS &
wait
python submit.py bin/server.static.exe bin/client.static.exe ${TOKEN}
