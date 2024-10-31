#!/usr/bin/env python3

import http.server
import os

# start the server
print('starting server...')
class SelfServer(http.server.ThreadingHTTPServer):
	def finish_request(self, request, client_address):
		self.RequestHandlerClass(request, client_address, self, directory=os.path.split(os.path.realpath(__file__))[0])
httpd = SelfServer(('localhost', 8080), http.server.SimpleHTTPRequestHandler)
print('server started at localhost:8080')
httpd.serve_forever()
