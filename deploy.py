#!/usr/bin/env python3

import http.server
import os

# ensure the generated directory exists
dirGenerated = os.path.join(os.path.split(__file__)[0], './server/.generated')
if not os.path.isdir(dirGenerated):
	os.mkdir(dirGenerated)

# compile the main application and export the relevant functions
print('compiling...')
if os.system('em++ -std=c++20'
			 # ensure that only wasm is generated without _start or other main wrapper
			 # 	functionality, which otherwise imports additional functionality
			 ' -o server/main.wasm'
			 ' -O1'
			 ' -sSTANDALONE_WASM'
			 ' --no-entry'
			 ' -DEMSCRIPTEN_COMPILATION'

			 # enable memory-heap growing possibility
			 ' -sALLOW_MEMORY_GROWTH'

			 # ensure exported functions can use i64, instead of it being split into two i32's
			 ' -sWASM_BIGINT'

			 # list of all exported functions
			 ' -sEXPORTED_FUNCTIONS='
			 '_startup,'
			 '_mem_mmap,'
			 '_mem_munmap,'
			 '_mem_mprotect,'
			 '_mem_perform_lookup,'
			 '_mem_result_physical,'
			 '_mem_result_size'

			 # add the relevant include directories
			 ' -Irepos'

			 # include the source files
			 ' main.cpp'
			 ' repos/wasgen/objects/wasm-module.cpp'
			 ' repos/wasgen/sink/wasm-target.cpp'
			 ' repos/wasgen/sink/wasm-sink.cpp'
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
			) != 0:
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
