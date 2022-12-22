#!/bin/bash
set -ex
CFLAGS="-static -Ofast"
g++ src/server.cpp -o bin/server.static.exe $CFLAGS &
g++ src/client.cpp -o bin/client.static.exe $CFLAGS &
python submit.py bin/server.static.exe bin/client.static.exe
