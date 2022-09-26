import dpkt
from base64 import b64decode
from sys import argv

def packet2ip(packet):
    parser = [
        dpkt.ethernet.Ethernet,
        dpkt.sll2.SLL2,
    ]
    for func in parser:
        pack = func(packet)
        if isinstance(pack.data, dpkt.ip.IP):
            return pack.data

with open(argv[1],'rb') as f:
    pcap = list(dpkt.pcap.UniversalReader(f))
    pcap = [packet2ip(pkt) for ts,pkt in pcap]
    pcap = [x for x in pcap if x]

mp = {}
for ip in pcap:
    k = (ip.tos,ip.ttl)
    if k not in mp:
        mp[k] = 0
    mp[k] += 1

for ip in pcap:
    k = (ip.tos,ip.ttl)
    if mp[k] == 1 and ip.data.data:
        text = b64decode(ip.data.data).decode()
        print(text)
