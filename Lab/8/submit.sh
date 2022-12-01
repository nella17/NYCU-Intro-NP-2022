#!/bin/bash
g++ -static src/server.cpp -o bin/server.exe &
g++ -static src/client.cpp -o bin/client.exe &
wait
python submit.py bin/server.exe bin/client.exe ${TOKEN?"TOKEN"}
