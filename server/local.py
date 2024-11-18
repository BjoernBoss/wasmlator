#!/usr/bin/env python3

import http.server
import os

# start the server
print('starting server...')
class SelfRequest(http.server.SimpleHTTPRequestHandler):
	def end_headers(self):
		self.send_header('Cache-Control', 'no-cache')
		super().end_headers()

class SelfServer(http.server.ThreadingHTTPServer):
	def finish_request(self, request, client_address):
		self.RequestHandlerClass(request, client_address, self, directory=os.path.split(os.path.realpath(__file__))[0])
httpd = SelfServer(('localhost', 9090), SelfRequest)
print('server started at localhost:9090')
httpd.serve_forever()
