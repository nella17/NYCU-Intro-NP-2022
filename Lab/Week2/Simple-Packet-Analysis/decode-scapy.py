from scapy.all import *
from base64 import b64decode
from sys import argv

pkts = rdpcap(argv[1])
data = [Ether(raw(x)) for x in pkts]

mp = {}
for e in data:
    k = (e.tos,e.ttl)
    if k not in mp:
        mp[k] = 0
    mp[k] += 1

for e in data:
    k = (e.tos,e.ttl)
    if mp[k] == 1:
        data = bytes(e.getlayer('TCP'))
        try:
            text = b64decode(data).decode()
            print(text)
        except:
            pass
