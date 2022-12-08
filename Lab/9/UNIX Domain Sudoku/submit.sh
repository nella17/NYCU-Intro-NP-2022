#!/bin/bash
g++ solver.cpp -o solver.exe -static &
python play.py solver.exe
