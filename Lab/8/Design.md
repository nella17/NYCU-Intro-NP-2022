# Protocol Design
- The protocol should support multiply connection at same time for server/client
- Easy to parse!

## Packet Format

### Session Initialzation
```
| Session ID (2) | Data Sequence = 0 (2) | Data Checksum (alder32) (hdr no included) | filename (4) | file size (4) |
```

- Steps
    - Sender send initialization request
    - Receiver send back a ACK response with `sequence no. = 0`
    - Sender start sending data using `sender transfer` format

### Data Transfer Format
```
| Session ID (2) | Data Sequence (2) | Data Checksum (4) | Data |
```
- Client timeout and resend when packet is somehow lost forever
    - 500ms
- When data is fully received by the serer
    - Server send End of Transmission
- When data is corrupted, server drop the packet and response a Malformed Data

#### Checksum algorithm
- Mainly alder32, but some xor operation is included to protect the important information like session ID or packet sequence
```c
    uint32_t value = adler32(hdr->data, DATA_SIZE);
    return value ^ hdr->data_seq ^ hdr->sess_id;
```

### Response Format

```
| Data Sequence (4) | Checksum (4) | Status Flag (4) |
```
- Status Flag
    - 0x1: ACK (Data received for certain packet sequence no.)
    - 0x2: Malformed Data (Checksum not match)
    - 0x3: End of transmission (All data received)
    - 0x4: RST for invalid session (received transfer with no session setup)

#### Checksum algorithm
- The flag is xored with a shared magic number
