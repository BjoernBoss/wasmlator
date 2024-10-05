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
		memory.setupCoreModule(_module);

		_module.close();
		const std::vector<uint8_t>& data = _writer.output();
		ctx.setCore(data.data(), data.size());

		//std::fstream _out{ "./temp-output.wasm", std::ios::out | std::ios::binary };
		//_out.write(reinterpret_cast<const char*>(data.data()), data.size());
	}

	{
		writer::TextWriter _writer;
		wasm::Module _module{ &_writer };
		memory.setupCoreModule(_module);

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
