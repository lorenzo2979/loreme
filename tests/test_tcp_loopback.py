#!/usr/bin/env python3
"""TCP integration smoke test using a host-only local mock. Never contacts GW006."""
import subprocess
import socket
import struct
import zlib
from pathlib import Path

BASE=Path(__file__).resolve().parents[1]

def packet(cmd,seq,body=b''):
    prefix=b'GWIR'+bytes((3,cmd,0,0))+struct.pack('!II',seq,len(body))
    crc=zlib.crc32(prefix+body)
    return prefix+struct.pack('!I',crc)+body

def exact(s,n):
    buf=bytearray()
    while len(buf)<n:
        chunk=s.recv(n-len(buf))
        if not chunk: raise RuntimeError('premature close')
        buf+=chunk
    return bytes(buf)

def receive(s,expect_seq):
    header=exact(s,20)
    assert header[:4]==b'GWIR' and header[4]==3
    cmd=header[5]
    seq,count=struct.unpack('!II',header[8:16])
    assert seq==expect_seq and count<=8208
    body=exact(s,count)
    assert zlib.crc32(header[:16]+body)==struct.unpack('!I',header[16:20])[0]
    return cmd,body

p=subprocess.Popen([str(BASE/'build/mock_tcp_server')],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
try:
    port=int(p.stdout.readline().strip())
    with socket.create_connection(('127.0.0.1',port),timeout=5) as s:
        s.settimeout(5)
        req=packet(1,1)
        for b in req: s.sendall(bytes((b,)))
        c,b=receive(s,1)
        assert c==0x81 and b[0]==3 and b[4]&4
        s.sendall(packet(2,2)+packet(7,3))
        assert receive(s,2)==(0x90,b'\x00')
        cmd,body=receive(s,3)
        assert cmd==0x87 and body[1]==1
        s.sendall(packet(3,4))
        assert receive(s,4)==(0x90,b'\x00')
        s.sendall(packet(5,5))
        cmd,body=receive(s,5)
        assert cmd==0x85
        capture_id=struct.unpack('!I',body[:4])[0]
        assert capture_id>0
        assert struct.unpack('!IHHI',body[4:16])==(38000,333,4,25000)
        assert struct.unpack('!IIII',body[16:])==(9000,4500,560,560)
        # Simulate Wi-Fi failure: the client disappears before persisting / ACK.
    with socket.create_connection(('127.0.0.1',port),timeout=5) as s:
        s.settimeout(5)
        s.sendall(packet(5,6))
        cmd,body=receive(s,6)
        assert cmd==0x85 and struct.unpack('!I',body[:4])[0]==capture_id
        # Incorrect token cannot discard the capture.
        s.sendall(packet(8,7,struct.pack('!I',capture_id+7)))
        assert receive(s,7)==(0x90,b'\x03')
        s.sendall(packet(8,8,struct.pack('!I',capture_id)))
        assert receive(s,8)==(0x90,b'\x00')
        s.sendall(packet(8,9,struct.pack('!I',capture_id)))
        assert receive(s,9)==(0x90,b'\x00')
        s.sendall(packet(5,10))
        assert receive(s,10)==(0x90,b'\x03')
        frame=struct.pack('!IHHIIIII',38000,333,4,60000,9000,4500,560,560)
        s.sendall(packet(4,11,frame))
        assert receive(s,11)==(0x90,b'\x00')
    assert p.wait(timeout=5)==0
    print('PASS loopback TCP: framing, reconnect, repeated read, ACK, stale/duplicate ACK, TX')
finally:
    if p.poll() is None:
        p.kill(); p.wait(timeout=5)
