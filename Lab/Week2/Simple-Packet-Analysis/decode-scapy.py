from scapy.all import *
from base64 import b64decode
from sys import argv

pkts = rdpcap(argv[1])

mp = {}
for p in pkts:
    k = (p.tos,p.ttl)
    if k not in mp:
        mp[k] = 0
    mp[k] += 1

# print(mp)

for p in pkts:
    k = (p.tos,p.ttl)
    if mp[k] == 1:
        data = p.getlayer('Raw')
        if data:
            text = b64decode(bytes(data)).decode()
            print(text)
