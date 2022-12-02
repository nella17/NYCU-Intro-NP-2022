#!/bin/bash
for i in {0..499}; do
  (cat /dev/random | head -c 8192 > ./files/send/$(printf "%06d" $i)) &
done
wait
