#!/bin/bash
set -x

CFLAGS="-g"
g++ src/server.cpp -o bin/server.exe $CFLAGS &
g++ src/client.cpp -o bin/client.exe $CFLAGS &
wait
