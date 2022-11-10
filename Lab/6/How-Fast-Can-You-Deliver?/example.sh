#!/bin/sh -x
echo '/ping' | nc localhost 9998 | head -n 1
echo '/reset' | nc localhost 9998 | head -n 1
(timeout 10 nc localhost 9999 &)
(timeout 10 nc localhost 9999 &)
(timeout 10 nc localhost 9999 &)
echo '/clients' | nc localhost 9998 | head -n 1
echo 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' | timeout 1 nc localhost 9999
echo 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' | timeout 1 nc localhost 9999
echo 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' | timeout 1 nc localhost 9999
sleep 1
echo '/report' | nc localhost 9998 | head -n 1
