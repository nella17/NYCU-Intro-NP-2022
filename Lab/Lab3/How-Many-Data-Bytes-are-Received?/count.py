from pwn import *

io = remote('inp111.zoolab.org', 10002)

io.readuntil(b'start.\n')

io.sendline(b'GO')
io.readuntil(b'==== BEGIN DATA ====\n')
data = io.readuntil(b'==== END DATA ====\n', drop=True)

cnt = len(data) - 1
info(f'received {cnt} bytes')

io.sendline(str(cnt).encode())

io.interactive()
