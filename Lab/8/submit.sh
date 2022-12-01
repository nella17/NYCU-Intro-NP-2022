#!/bin/bash
set -ex
g++ -static src/server.cpp -o bin/server.static.exe &
g++ -static src/client.cpp -o bin/client.static.exe &
wait
python submit.py bin/server.static.exe bin/client.static.exe ${TOKEN?"TOKEN"}
