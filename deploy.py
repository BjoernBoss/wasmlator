#!/usr/bin/env python3

import http.server
import os

# compile the script and export the relevant functions and main function
print('compiling...')
if os.system('em++ -std=c++20'
			 ' -o server/main.html -O1 -DEMSCRIPTEN_COMPILATION'
			 ' -Irepos'
			 ' main.cpp'
			 ' repos/wasgen/wasm/wasm-module.cpp'
			 ' repos/wasgen/wasm/wasm-target.cpp'
			 ' repos/wasgen/wasm/wasm-sink.cpp'
			 ' --js-library ./interface/emscripten-interface.js'
			 ' --pre-js ./interface/emscripten-pre-js.js'
			 ' interface/native-interface.cpp'
			 ' interface/wasm-interface.cpp'

			 ' repos/wasgen/writer/text/text-base.cpp'
			 ' repos/wasgen/writer/text/text-module.cpp'
			 ' repos/wasgen/writer/text/text-sink.cpp'

			 ' repos/wasgen/writer/binary/binary-base.cpp'
			 ' repos/wasgen/writer/binary/binary-module.cpp'
			 ' repos/wasgen/writer/binary/binary-sink.cpp'

			 ' --js-library env/context/bridge-context-js-imports.js'
			 ' --pre-js env/context/bridge-context-js-predefined.js'
			 ' env/context/bridge-context.cpp'
			 ' env/context/env-context.cpp'

			 ' --js-library env/memory/bridge-memory-js-imports.js'
			 ' --pre-js env/memory/bridge-memory-js-predefined.js'
			 ' env/memory/env-memory.cpp'
			 ' env/memory/memory-bridge.cpp'
			 ' env/memory/memory-mapper.cpp'
			 ' env/memory/memory-interaction.cpp'

			 ' -sEXPORTED_FUNCTIONS='
			 '_startup,'
			 '_mem_mmap,'
			 '_mem_munmap,'
			 '_mem_mprotect,'
			 '_mem_perform_lookup,'
			 '_mem_result_physical,'
			 '_mem_result_size'

			# ensure exported functions can use i64, instead of it being split into two i32's
			 ' -sWASM_BIGINT') != 0:
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
