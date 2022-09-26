import dpkt
from base64 import b64decode
from sys import argv

with open(argv[1],'rb') as f:
    pcap = list(dpkt.pcap.UniversalReader(f))

mp = {}
for ts,pkt in pcap:
    eth = dpkt.ethernet.Ethernet(pkt)
    if not isinstance(eth.data, dpkt.ip.IP):
        continue
    ip = eth.data
    k = (ip.tos,ip.ttl)
    if k not in mp:
        mp[k] = 0
    mp[k] += 1

for ts,pkt in pcap:
    eth = dpkt.ethernet.Ethernet(pkt)
    if not isinstance(eth.data, dpkt.ip.IP):
        continue
    ip = eth.data
    k = (ip.tos,ip.ttl)
    if mp[k] == 1 and ip.data.data:
        text = b64decode(ip.data.data).decode()
        print(text)
