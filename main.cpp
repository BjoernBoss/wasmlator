#include <wasgen/wasm/wasm.h>
#include <wasgen/writer/text-writer.h>
#include "util/logging.h"
#include "env/env-process.h"

extern "C" int test() {
	return 50;
}

int main() {
	writer::TextWriter _writer;

	{
		env::Process _proc = env::Process::Create(u8"test-proc");


	}

	{
		namespace I = wasm::inst;

		wasm::Module _module{ &_writer };

		for (wasm::Prototype proto : _module.prototypes()) {

		}

		for (wasm::Function fn : _module.functions()) {

		}

		wasm::Prototype i32_i32 = _module.prototype({ { u8"v", wasm::Type::i32 } }, { wasm::Type::i32 }, u8"i32_32");

		wasm::Function _fn = _module.function(i32_i32, u8"abc_def", {}, wasm::Export{ u8"test" });
		wasm::Sink _sink = { _fn };
		wasm::Memory _mem = _module.memory(wasm::Limit{ 1024 }, u8"def-memory", wasm::Import{ u8"env", u8"primary-memory" });

		_sink[I::I32::Const(50)];
		//_sink[wasm::Inst<TextWriter>::Add()];
		{
			wasm::IfThen _if{ _sink, u8"conditional" };

			wasm::Variable _local{ _sink.local(wasm::Type::i32, u8"test-var") };

			wasm::Block _block0{ _sink, u8"block0" };
			wasm::Block _block1{ _sink, u8"block1" };
			wasm::Block _block2{ _sink, u8"block2" };

			_sink[I::I32::Load8(_mem, 0)];
			_sink[I::I32::Load8(_mem, 0)];

			_sink[I::Branch::Table({ _block0, _block1, _block2 }, _if)];

			_if.otherwise();


			_sink[I::I32::Load8(_mem, 0)];

			_sink[I::F32::Min()];
			_sink[I::U32::Expand()];

			_sink[I::U64::Const(12315)];
			_sink[I::U64::Equal()];

			_if.close();
		}
		_sink[I::I32::Load8(_mem, 0)];
	}

	str::PrintLn(_writer.output());

	util::log(u8"Hello, from this log!");
	util::fail(u8"failure!");
	return 0;
}
