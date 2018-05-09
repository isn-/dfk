#!/usr/bin/env python
import sys
try:
    import SocketServer as socketserver
except ImportError:
    import socketserver

host = sys.argv[0]
port = int(sys.argv[1])

class EchoTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        data = self.request.recv(1024)
        self.request.sendall(data)

server = socketserver.TCPServer((host, port), EchoTCPHandler)
server.serve_forever()
