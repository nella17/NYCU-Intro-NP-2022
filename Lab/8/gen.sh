#!/bin/bash
for i in {0..999}; do
  (cat /dev/random | head -c 65536 > ./files/send/$(printf "%06d" $i)) &
done
wait
