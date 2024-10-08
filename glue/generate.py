#!/usr/bin/env python3

import os

# ensure the generated directory exists
dirGenerated = os.path.join(os.path.split(__file__)[0], '../generated')
if not os.path.isdir(dirGenerated):
	os.mkdir(dirGenerated)

# compile the glue generator and produce the generator
print('compiling...')
if os.system('cd ' + os.path.split(__file__)[0] + ' &&'
			 ' clang++ -std=c++20'
			 ' -o ../generated/generate-glue.exe -O1'
			 ' -I../repos'
			 ' generate.cpp'
			 ' context-functions.cpp'
			 ' host-functions.cpp'
			 ' initialize-state.cpp'
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
print('compiled')

# execute the generator to produce its output
print('generating...')
if os.system('cd ' + dirGenerated + ' && generate-glue.exe') != 0:
	print('generation failed')
	exit(1)
print('generated')
