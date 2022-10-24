#!/bin/sh
set -x
f=${1-./tcpcbr.exe}
timeout 20 $f 1;   sleep 5  # send at 1 MBps
timeout 20 $f 1.5; sleep 5  # send at 1.5 MBps
timeout 20 $f 2;   sleep 5  # send at 2 MBps
timeout 20 $f 3
