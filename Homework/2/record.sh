#!/bin/bash
set -x
sudo tcpdump -q -s 0 -n -i lo -X -w dump.pcap -A port 53 &
reset
sleep 0.5
echo
dig +short google.com
dig +short google.com A
dig +short google.com AAAA
dig +short google.com NS
dig +short google.com CNAME
dig +short google.com SOA
dig +short google.com MX
dig +short google.com TXT
dig +short x.google.com
dig +short q.proto.ctf.nella17.tw
dig +short q.proto.ctf.nella17.tw CNAME
jobs
kill -SIGTERM %
