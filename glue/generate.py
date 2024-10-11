#!/usr/bin/env python3

import os

# ensure the generated directory exists
dirGenerated = os.path.join(os.path.split(__file__)[0], '../server/generated')
if not os.path.isdir(dirGenerated):
	os.mkdir(dirGenerated)

# setup the paths
programPath = os.path.abspath(os.path.join(dirGenerated, './make-glue.exe'))
wasmPath = os.path.abspath(os.path.join(dirGenerated, './glue-module.wasm'))
watPath = os.path.abspath(os.path.join(dirGenerated, './glue-module.wat'))

# compile the glue generator and produce the generator
print('compiling...')
if os.system(f'cd {os.path.split(__file__)[0]} &&'
			 ' clang++ -std=c++20'
			 ' -O1'
			 f' -o {programPath}'
			 ' -I../repos'
			 ' make-glue.cpp'
			 ' generate.cpp'
			 ' context-functions.cpp'
			 ' host-functions.cpp'
			 ' initialize-state.cpp'
			 ' blocks-functions.cpp'
			 ' memory-functions.cpp'

			 ' ../repos/wasgen/objects/wasm-module.cpp'
			 ' ../repos/wasgen/sink/wasm-target.cpp'
			 ' ../repos/wasgen/sink/wasm-sink.cpp'
			 ' ../repos/wasgen/writer/text/text-base.cpp'
			 ' ../repos/wasgen/writer/text/text-module.cpp'
			 ' ../repos/wasgen/writer/text/text-sink.cpp'
			 ' ../repos/wasgen/writer/binary/binary-base.cpp'
			 ' ../repos/wasgen/writer/binary/binary-module.cpp'
			 ' ../repos/wasgen/writer/binary/binary-sink.cpp'
			) != 0:
	print('compilation failed')
	exit(1)
print(f'compiled at: [{programPath}]')

# execute the generator to produce its output
print('generating...')
if os.system(f'cd {dirGenerated} && make-glue.exe {wasmPath} {watPath}') != 0:
	print('generation failed')
	exit(1)
print('generation completed')
