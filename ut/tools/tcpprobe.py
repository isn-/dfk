#!/usr/bin/env python
import sys
from time import sleep
import socket

host = sys.argv[0]
port = int(sys.argv[1])

def probe(host, port):
    for i in range(10):
        try:
            socket.create_connection((host, port))
            return True
        except ConnectionRefusedError:
            pass
        sleep(0.01 * (2 ** i))
    return False

if not probe(host, port):
    raise Exception("Can not connect to" + host + ":" + port)
