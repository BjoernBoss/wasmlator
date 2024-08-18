#include <wasgen/wasm.h>
#include <wasgen/text-writer.h>

#include "interface/interface.h"

#include <iostream>

extern "C" int test() {
	return 50;
}

int main() {
	{
		using I = wasm::Inst<TextWriter>;

		wasm::Module<TextWriter> _module{ TextWriter{} };

		wasm::Prototype i32_i32 = _module.prototype(u8"i32_32", { { u8"v", wasm::Type::i32 } }, { wasm::Type::i32 });

		wasm::Function _fn = _module.function(u8"abc_def", i32_i32, wasm::Export{ u8"test" });
		wasm::Sink<TextWriter> _sink = _module.sink(_fn);
		wasm::Memory<TextWriter> _mem = _module.memory(u8"def-memory", wasm::Limit{ 1024 }, wasm::Import{ u8"env", u8"primary-memory" });

		_sink[I::I32::Const(50)];
		//_sink[wasm::Inst<TextWriter>::Add()];
		{
			wasm::IfThen _if{ _sink };

			_if.otherwise();

			_if.close();
		}
		_sink[I::I32::Load8(_mem, 0)];
		//str::PrintLn(_fn);
	}

	env::log(u8"Hello, from this log!");
	env::fail(u8"failure!");
	return 0;
}
