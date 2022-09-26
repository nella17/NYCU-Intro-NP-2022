import dpkt
from base64 import b64decode
from sys import argv

def pcap2ips(pcap):
    parsers = {
        dpkt.pcap.DLT_EN10MB:       dpkt.ethernet.Ethernet,
        dpkt.pcap.DLT_LINUX_SLL2:   dpkt.sll2.SLL2,
    }
    parser = parsers.get( pcap.datalink() )
    if parser is None:
        raise NotImplementedError
    res = []
    for ts, packet in pcap:
        ip = parser(packet).data
        res.append(ip)
    return res

with open(argv[1],'rb') as f:
    pcap = dpkt.pcap.UniversalReader(f)
    pcap = pcap2ips(pcap)

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
