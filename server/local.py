#!/usr/bin/env python3

import http.server
import os
import stat
import json

# file-system interaction class (WARNING: no validation of paths)
class FileSystemInteract:
	def __init__(self, path):
		self._root = path
	def _path(self, path):
		if path.startswith('/'):
			path = path[1:]
		return os.path.join(self._root, path)
	def getStats(self, path):
		path = self._path(path)

		# fetch the file-stats and format them
		try:
			s = os.lstat(path)
		except:
			return None
		out = {}
		out['link'] = ''
		out['size'] = 0
		out['atime_us'] = s.st_atime_ns // 1000
		out['mtime_us'] = s.st_mtime_ns // 1000

		# check if the entry is a symbolic link
		if stat.S_ISLNK(s.st_mode):
			out['link'] = os.readlink(path)
			out['type'] = 'link'
			out['size'] = len(out['link'].encode('utf-8'))
		
		# check if the entry is a file
		elif stat.S_ISREG(s.st_mode):
			out['size'] = s.st_size
			out['type'] = 'file'

		# check if the entry is a directory
		elif stat.S_ISDIR(s.st_mode):
			out['type'] = 'dir'

		# unknown object encountered
		else:
			return None
		return out
	def getSize(self, path):
		path = self._path(path)
		return os.lstat(path).st_size
	def open(self, path):
		path = self._path(path)
		return open(path, 'rb')

# root directory of the server
rootPath = os.path.split(os.path.realpath(__file__))[0]
fileSystem = FileSystemInteract(os.path.join(rootPath, './fs'))
staticPath = os.path.join(rootPath, './static')

# request handler implementation
class SelfRequest(http.server.SimpleHTTPRequestHandler):
	def do_stats(self, body):
		out = json.dumps(fileSystem.getStats(self.path[6:])).encode('utf-8')
		super().send_response(http.HTTPStatus.OK)
		super().send_header('Content-Length', f'{len(out)}')
		super().send_header('Content-Type', 'application/json; charset=utf-8')
		super().end_headers()
		if body:
			self.wfile.write(out)

	def do_read(self, body):
		super().send_response(http.HTTPStatus.OK)
		super().send_header('Content-Length', f'{fileSystem.getSize(self.path[6:])}')
		super().send_header('Content-Type', 'application/binary')
		super().end_headers()
		if not body:
			return None
		try:
			f = fileSystem.open(self.path[5:])
			super().copyfile(f, self.wfile)
		finally:
			f.close()

	def dispatch(self, body):
		# check if the request should be handled separately
		if self.path.startswith('/stat/'):
			self.do_stats(body)
			return None
		elif self.path.startswith('/data/'):
			self.do_read(body)
			return None

		# dispatch the path to be used
		if self.path.startswith('/wasm/') or self.path.startswith('/wat/'):
			return rootPath
		return staticPath

	def do_GET(self):
		self.directory = self.dispatch(True)
		if self.directory is None:
			return
		super().do_GET()

	def do_HEAD(self):
		self.directory = self.dispatch(False)
		if self.directory is None:
			return
		super().do_HEAD()

	def end_headers(self):
		self.send_header('Cache-Control', 'no-cache')
		super().end_headers()

# local server implementation
class SelfServer(http.server.ThreadingHTTPServer):
	def finish_request(self, request, client_address):
		self.RequestHandlerClass(request, client_address, self)

# configure the http-server daemon and start it
print('starting server...')
httpd = SelfServer(('localhost', 9090), SelfRequest)
print('server started at localhost:9090')
httpd.serve_forever()
