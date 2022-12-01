#!/bin/bash
g++ -g src/server.cpp -o bin/server &
g++ -g src/client.cpp -o bin/client &
wait
