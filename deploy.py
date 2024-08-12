#!/usr/bin/env python3

import http.server
import os

# compile the script and export the '_test' function
print('compiling...')
if os.system('em++ -std=c++20 --js-library ./interface.js main.cpp -o server/main.html -s EXPORTED_FUNCTIONS=_test -O1 -DEMSCRIPTEN_COMPILATION') != 0:
	exit(1)
print('compiled')

# start the server
print('starting server...')
class SelfServer(http.server.ThreadingHTTPServer):
	def finish_request(self, request, client_address):
		self.RequestHandlerClass(request, client_address, self, directory='./server')
httpd = SelfServer(('localhost', 8080), http.server.SimpleHTTPRequestHandler)
print('server started at localhost:8080')
httpd.serve_forever()
