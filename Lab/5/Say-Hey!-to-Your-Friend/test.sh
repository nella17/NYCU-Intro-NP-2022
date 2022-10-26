#!/bin/bash
MAX=1000
I=0
while [ "$I" -lt "$MAX" ]; do
  I=$((I+1));
  (timeout 30 nc localhost 10005 >/dev/null 2>&1 &)
done
wait
