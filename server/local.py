#!/usr/bin/env python3

import http.server
import os
import stat
import json

# file-system interaction class (WARNING: no validation of paths)
class FileSystemInteract:
	def __init__(self, path):
		self._path = path
	def getStats(self, path):
		# construct the actual path
		if path.startswith('/'):
			path = path[1:]
		path = os.path.join(self._path, path)

		# fetch the file-stats and format them
		try:
			s = os.lstat(path)
		except:
			return None
		out = {}
		out['name'] = os.path.split(path)[1]
		out['link'] = ''
		out['size'] = 0
		out['atime'] = s.st_atime_ns
		out['mtime'] = s.st_mtime_ns

		# check if the entry is a symbolic link
		if stat.S_ISLNK(s.st_mode):
			out['link'] = os.readlink()
			out['type'] = 3 # link
		
		# check if the entry is a file
		elif stat.S_ISREG(s.st_mode):
			out['size'] = s.st_size
			out['type'] = 1 # file

		# check if the entry is a directory
		elif stat.S_ISDIR(s.st_mode):
			out['type'] = 2 # directory
		
		# unknown object encountered
		else:
			return None
		return out

# root directory of the server
rootPath = os.path.split(os.path.realpath(__file__))[0]
fileSystem = FileSystemInteract(os.path.join(rootPath, './fs'))

# request handler implementation
class SelfRequest(http.server.SimpleHTTPRequestHandler):
	def do_stats(self):
		out = json.dumps(fileSystem.getStats(self.path[6:])).encode('utf-8')
		super().send_response(http.HTTPStatus.OK)
		super().send_header('Content-Length', f'{len(out)}')
		super().send_header('Content-Type', 'application/json; charset=utf-8')
		super().end_headers()
		return out

	def do_GET(self):
		# check if the request should be handled separately
		if self.path.startswith('/stat/'):
			out = self.do_stats()
			try:
				self.wfile.write(out)
			except:
				pass
		else:
			return super().do_GET()

	def do_HEAD(self):
		# check if the request should be handled separately
		if self.path.startswith('/stat/'):
			self.do_stats()
		else:
			return super().do_HEAD()

	def end_headers(self):
		self.send_header('Cache-Control', 'no-cache')
		super().end_headers()

# local server implementation
class SelfServer(http.server.ThreadingHTTPServer):
	def finish_request(self, request, client_address):
		self.RequestHandlerClass(request, client_address, self, directory=rootPath)

# configure the http-server daemon and start it
print('starting server...')
httpd = SelfServer(('localhost', 9090), SelfRequest)
print('server started at localhost:9090')
httpd.serve_forever()
