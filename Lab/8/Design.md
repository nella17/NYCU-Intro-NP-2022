# Protocol Design
## Packet Format

### Initialize connection
```
| Data Sequence = 0 (4) | Data Checksum (alder32) (hdr no included) | filename (4) | file size (4) |
```

- Steps
    - Sender send this packet
    - Receiver send back a ACK response with `sequence no. = 0`
    - Sender start sending data using sender transfer format

### Sender Transfer
```
| Data Sequence (4) | Data Checksum (alder32) (hdr no included) | Data |
```

- Sending timeout
    - 500ms
- When received data length with 0
    - End of transmission

### Receiver

```
| Data Sequence (4) | Checksum (4) | Status Flag (4) |
```
- Status Flag
    - 0x1: ACK (Data received for certain packet sequence no.)
    - 0x2: Malformed Data (Checksum not match)
    - 0x3: End of transmission (All data received)