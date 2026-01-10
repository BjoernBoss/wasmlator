# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026 Bjoern Boss Henrichsen
#!/usr/bin/env python3

import http.server
import os
import sys
import stat
import json

# file-system interaction class
class FileSystemInteract:
	def __init__(self, path):
		self._root = self._makeRealPath(path)
	def _makeRealPath(self, path):
		path = os.path.realpath(path)
		if sys.platform.startswith("win"):
			if path.startswith('\\\\?\\'):
				path = path[4:]
			return path.lower()
		return path
	def _validatePath(self, path):
		# check if its the root path
		if path == '/':
			return self._root, []

		# validate the path itself to be absolute and canonicalized
		if not path.startswith('/') or '\\' in path:
			return None, []
		if path.endswith('/'):
			path = path[:-1]
		parts = path[1:].split('/')
		if ('' in parts) or ('.' in parts) or ('..' in parts):
			return None, []

		# iterate over the path components and ensure that each is a valid directory (i.e. not a symlink)
		actual = self._root
		for part in parts[:-1]:
			actual = os.path.join(actual, part)
			if os.path.islink(actual) or not os.path.isdir(actual):
				return None, []
		return os.path.join(actual, parts[-1]), parts
	def _patchLink(self, parts, link):
		# check if the link is absolute and matches the base-path of the file-system
		if os.path.isabs(link):
			try:
				link = self._makeRealPath(link)
				if not link.startswith(self._root) or link[len(self._root)] not in '/\\':
					return None
				return '/' + os.path.relpath(link, self._root).replace('\\', '/')
			except:
				return None

		# check if the path is relative but does not leave the root directory
		link = link.replace('\\', '/')
		if len(link) == 0 or link.startswith('/'):
			return None
		if link.endswith('/'):
			link = link[:-1]
		lparts = link.split('/')

		# apply the relative changes to the current parts into the current directory
		for part in lparts:
			if part == '':
				return None
			if part == '.':
				continue
			if part != '..':
				parts.append(part)
			elif len(parts) == 0:
				return None
			else:
				parts.pop()
		return link
	def getStats(self, path):
		path, parts = self._validatePath(path)
		if path is None:
			return None

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
			link = self._patchLink(parts, os.readlink(path))
			if link is None:
				return None
			out['link'] = link
			out['type'] = 'link'
			out['size'] = len(link.encode('utf-8'))
		
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
	def getList(self, path):
		base = (path if path == '/' else f'{path}/')
		path, _ = self._validatePath(path)
		if path is None or os.path.islink(path) or not os.path.isdir(path):
			return None
		
		return [x for x in os.listdir(path) if x not in ['', '.', '..'] and self.getStats(base + x) is not None]
	def open(self, path):
		path, _ = self._validatePath(path)
		if path is None or os.path.islink(path) or not os.path.isfile(path):
			return None
		return open(path, 'rb')
	def getSize(self, f):
		return os.fstat(f.fileno()).st_size

# fetch the root directory to be used
pathFs = open('{{root-indicator-path}}', 'r').read().strip()
pathFs = os.path.realpath(os.path.join('{{exec-path}}', pathFs))
if not os.path.isdir(pathFs):
	print(f'error: filesystem root [{pathFs}] is not a directory')
	exit(1)
print(f'filesystem root at [{pathFs}]')

# root directory of the server
fileSystem = FileSystemInteract(pathFs)
pathCommon = os.path.realpath('{{common-path}}')
pathWasm = os.path.realpath('{{wasm-path}}')
pathWeb = os.path.realpath('{{self-path}}')

# request handler implementation
class SelfRequest(http.server.SimpleHTTPRequestHandler):
	def do_stats(self, body, path):
		stats = fileSystem.getStats(path)
		out = json.dumps(stats).encode('utf-8')
		super().send_response(http.HTTPStatus.OK)
		super().send_header('Content-Length', f'{len(out)}')
		super().send_header('Content-Type', 'application/json; charset=utf-8')
		super().end_headers()
		if body:
			self.wfile.write(out)
	def do_read(self, body, path):
		f = fileSystem.open(path)
		if f is None:
			super().send_error(http.HTTPStatus.NOT_FOUND, "File not found")
			return None
		super().send_response(http.HTTPStatus.OK)
		super().send_header('Content-Length', f'{fileSystem.getSize(f)}')
		super().send_header('Content-Type', 'application/binary')
		super().end_headers()
		if not body:
			return None
		try:
			super().copyfile(f, self.wfile)
		finally:
			f.close()
	def do_list(self, body, path):
		list = fileSystem.getList(path)
		out = json.dumps(list).encode('utf-8')
		super().send_response(http.HTTPStatus.OK)
		super().send_header('Content-Length', f'{len(out)}')
		super().send_header('Content-Type', 'application/json; charset=utf-8')
		super().end_headers()
		if body:
			self.wfile.write(out)
	def dispatch(self, body):
		# check if the request should be handled separately
		if self.path.startswith('/stat/'):
			self.do_stats(body, self.path[5:])
			return None
		elif self.path.startswith('/data/'):
			self.do_read(body, self.path[5:])
			return None
		elif self.path.startswith('/list/'):
			self.do_list(body, self.path[5:])
			return None

		# dispatch the path to be used
		if self.path.startswith('/wasm/'):
			self.path = self.path[5:]
			return pathWasm
		if self.path.startswith('/com/'):
			self.path = self.path[4:]
			return pathCommon
		return pathWeb
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
