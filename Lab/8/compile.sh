#!/bin/bash
g++ -g src/server.cpp -o bin/server.exe &
g++ -g src/client.cpp -o bin/client.exe &
wait
