import pyshark
from base64 import b64decode
from sys import argv

pcap = pyshark.FileCapture(argv[1])
target = '140.113.213.213'

mp = {}

for idx,pkt in enumerate(pcap):
    if 'TCP' not in pkt:
        continue
    if pkt.ip.src != target:
        continue
    k = (pkt.ip.ttl,pkt.ip.dsfield)
    if k not in mp:
        mp[k] = 0
    mp[k] += 1


for idx,pkt in enumerate(pcap):
    if 'TCP' not in pkt:
        continue
    if pkt.ip.src != target:
        continue
    k = (pkt.ip.ttl,pkt.ip.dsfield)
    if mp[k] == 1:
        try:
            data = bytes.fromhex(pkt.DATA.data)
            text = b64decode(data).decode()
            print(text)
        except:
            pass

