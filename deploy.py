#!/usr/bin/env python3

import http.server
import os

# ensure the generated directory exists
dirGenerated = os.path.join(os.path.split(__file__)[0], './server/generated')
if not os.path.isdir(dirGenerated):
	os.mkdir(dirGenerated)

# compile the main application and export the relevant functions
print('compiling...')
if os.system('em++ -std=c++20'
			 # ensure that only wasm is generated without _start or other main wrapper
			 # 	boilerplate code, which otherwise imports additional functionality
			 ' -o server/main.wasm'
			 ' -O1'
			 ' -sSTANDALONE_WASM'
			 ' --no-entry'

			 # enable memory-heap growing possibility
			 ' -sALLOW_MEMORY_GROWTH'

			 # ensure any undefined symbols are just silently translated to imports
			 ' -sERROR_ON_UNDEFINED_SYMBOLS=0'
			 ' -sWARN_ON_UNDEFINED_SYMBOLS=0'

			 # ensure exported functions can use i64, instead of it being split into two i32's
			 ' -sWASM_BIGINT'

			 # list of all exported functions
			 ' -sEXPORTED_FUNCTIONS='
			 '_main_startup,'
			 '_ctx_core_loaded,'
			 '_mem_mmap,'
			 '_mem_munmap,'
			 '_mem_mprotect,'
			 '_mem_perform_lookup,'
			 '_mem_result_physical,'
			 '_mem_result_size'
			 
			 # mark this to be the emscripten-build
			 ' -DEMSCRIPTEN_COMPILATION'

			 # add the relevant include directories
			 ' -Irepos'

			 # include the source files
			 ' main.cpp'
			 ' repos/wasgen/objects/wasm-module.cpp'
			 ' repos/wasgen/sink/wasm-target.cpp'
			 ' repos/wasgen/sink/wasm-sink.cpp'
			 ' repos/wasgen/writer/text/text-base.cpp'
			 ' repos/wasgen/writer/text/text-module.cpp'
			 ' repos/wasgen/writer/text/text-sink.cpp'
			 ' repos/wasgen/writer/binary/binary-base.cpp'
			 ' repos/wasgen/writer/binary/binary-module.cpp'
			 ' repos/wasgen/writer/binary/binary-sink.cpp'

			 ' interface/interface.cpp'
			 ' interface/entry-point.cpp'
			 ' util/logging.cpp'

			 ' env/env-process.cpp'

			 ' env/context/context-bridge.cpp'
			 ' env/context/env-context.cpp'

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
