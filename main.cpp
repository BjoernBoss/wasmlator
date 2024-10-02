#include <wasgen/wasm/wasm.h>
#include <wasgen/writer/binary-writer.h>
#include <wasgen/writer/text-writer.h>
#include "util/logging.h"
#include "env/memory/env-memory.h"

int main() {
	env::Context ctx{ u8"text_module" };
	env::Memory memory{ ctx, 4 };

	{
		writer::BinaryWriter _writer;
		wasm::Module _module{ &_writer };
		wasm::Global _glob1 = _module.global(u8"test1", wasm::Type::i32, true, u8"test");
		memory.setupCoreModule(_module);
		wasm::Memory _mem0 = _module.memory(u8"test-mem", wasm::Limit{ 1 });

		wasm::Global _glob0 = _module.global(u8"test0", wasm::Type::i32, true);

		_module.value(_glob0, wasm::Value::MakeImported(_glob1));
		_module.data(_mem0, wasm::Value::MakeU32(134), { 0x00, 0x23, 0x00, 0xff, 0x10, 0xa0, 0xb0 });

		_module.close();


		_module.close();
		const std::vector<uint8_t>& data = _writer.output();
		ctx.setCore(data.data(), data.size());

		std::fstream _out{ "./temp-output.wasm", std::ios::out | std::ios::binary };
		_out.write(reinterpret_cast<const char*>(data.data()), data.size());

		memory.read<uint64_t>(0x123);
	}

	{
		writer::TextWriter _writer;
		wasm::Module _module{ &_writer };
		wasm::Global _glob1 = _module.global(u8"test1", wasm::Type::i32, true, u8"test");
		memory.setupCoreModule(_module);
		wasm::Memory _mem0 = _module.memory(u8"test-mem", wasm::Limit{ 1 });

		wasm::Global _glob0 = _module.global(u8"test0", wasm::Type::i32, true);

		_module.value(_glob0, wasm::Value::MakeI32(50));
		_module.data(_mem0, wasm::Value::MakeU32(134), { 0x00, 0x23, 0x00, 0xff, 0x10, 0xa0, 0xb0 });

		_module.close();


		const std::u8string& data = _writer.output();
		std::fstream _out{ "./temp-output.wat", std::ios::out };
		_out.write(reinterpret_cast<const char*>(data.data()), data.size());
	}

	{
		writer::TextWriter _writer;
		wasm::Module _module{ &_writer };
		memory.setupImports(_module);

		_module.close();
		util::log(u8"----------------------------------------   imports   ----------------------------------------");
		util::log(_writer.output());
		util::log(u8"---------------------------------------------------------------------------------------------");
	}

	util::fail(u8"failure!");
	return 0;
}
