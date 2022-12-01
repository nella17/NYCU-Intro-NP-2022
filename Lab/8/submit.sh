#!/bin/bash
g++ -static src/server.cpp -o bin/server &
g++ -static src/client.cpp -o bin/client &
wait
python submit.py bin/server bin/client ${TOKEN?"TOKEN"}
