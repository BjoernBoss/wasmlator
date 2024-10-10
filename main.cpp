#include <wasgen/wasm.h>
#include <wasgen/writer/binary-writer.h>
#include <wasgen/writer/text-writer.h>
#include "util/logging.h"
#include "env/memory/env-memory.h"

extern "C" void startup() {
	env::Context ctx{ u8"text_module" };
	env::Memory memory{ ctx, 4 };

	writer::BinaryWriter _writer;
	wasm::Module _module{ &_writer };
	memory.setupCoreModule(_module);

	_module.close();
	const std::vector<uint8_t>& data = _writer.output();
	ctx.setCore(data.data(), data.size());

	memory.mmap(env::VirtPageSize * 4, env::VirtPageSize * 4, env::MemoryUsage::Read);
	memory.mmap(env::VirtPageSize * 8, env::VirtPageSize * 4, env::MemoryUsage::Write);
	memory.mprotect(env::VirtPageSize * 6, env::VirtPageSize * 4, env::MemoryUsage::Execute);
	for (size_t i = 4; i < 12; i += 2)
		memory.munmap(env::VirtPageSize * i, env::VirtPageSize);
	memory.mmap(env::VirtPageSize * 12, env::VirtPageSize * 4, env::MemoryUsage::Write);
	memory.mmap(env::VirtPageSize * 20, env::VirtPageSize * 4, env::MemoryUsage::Read);
	memory.mmap(env::VirtPageSize * 16, env::VirtPageSize * 4, env::MemoryUsage::Execute);
	memory.mmap(env::VirtPageSize * 6, env::VirtPageSize * 1, env::MemoryUsage::Read);
	memory.mmap(env::VirtPageSize * 10, env::VirtPageSize * 1, env::MemoryUsage::Execute);
	memory.mmap(env::VirtPageSize * 8, env::VirtPageSize * 1, env::MemoryUsage::Write);
	memory.mprotect(env::VirtPageSize * 8, env::VirtPageSize * 6, env::MemoryUsage::Read);
	memory.munmap(env::VirtPageSize * 15, env::VirtPageSize * 4);
	memory.mmap(env::VirtPageSize * 0, env::VirtPageSize * 2, env::MemoryUsage::Read);

	memory.mmap(env::VirtPageSize * 2, env::VirtPageSize * 2, 0);
	memory.mmap(env::VirtPageSize * 15, env::VirtPageSize * 4, env::MemoryUsage::Execute | env::MemoryUsage::Read);
	memory.mmap(env::VirtPageSize * 4, env::VirtPageSize * 1, env::MemoryUsage::Write);
	memory.mprotect(env::VirtPageSize * 1, env::VirtPageSize * 22, env::MemoryUsage::Read);

	memory.mmap(env::VirtPageSize * 26, env::VirtPageSize * 2, env::MemoryUsage::Read);
	memory.mmap(env::VirtPageSize * 28, env::VirtPageSize * 2, env::MemoryUsage::Write);
	memory.mprotect(env::VirtPageSize * 27, env::VirtPageSize * 2, env::MemoryUsage::Write);

	util::fail(u8"failure!");
}

int main() {
	startup();

	//env::Context ctx{ u8"text_module" };
	//env::Memory memory{ ctx, 4 };
	//
	//{
	//	writer::BinaryWriter _writer;
	//	wasm::Module _module{ &_writer };
	//	memory.setupCoreModule(_module);
	//
	//	_module.close();
	//	const std::vector<uint8_t>& data = _writer.output();
	//	ctx.setCore(data.data(), data.size());
	//
	//	// std::fstream _out{ "./temp-output.wasm", std::ios::out | std::ios::binary };
	//	// _out.write(reinterpret_cast<const char*>(data.data()), data.size());
	//}
	//
	//{
	//	writer::TextWriter _writer;
	//	wasm::Module _module{ &_writer };
	//	memory.setupCoreModule(_module);
	//
	//	_module.close();
	//	const std::u8string& data = _writer.output();
	//
	//	// std::fstream _out{ "./temp-output.wat", std::ios::out };
	//	// _out.write(reinterpret_cast<const char*>(data.data()), data.size());
	//}
	//
	//{
	//	writer::TextWriter _writer;
	//	wasm::Module _module{ &_writer };
	//	memory.setupBlockModule(_module);
	//
	//	_module.close();
	//	util::log(u8"----------------------------------------   imports   ----------------------------------------");
	//	util::log(_writer.output());
	//	util::log(u8"---------------------------------------------------------------------------------------------");
	//}
	//
	//memory.mmap(env::VirtPageSize * 4, env::VirtPageSize * 4, env::MemoryUsage::Read);
	//memory.mmap(env::VirtPageSize * 8, env::VirtPageSize * 4, env::MemoryUsage::Write);
	//memory.mprotect(env::VirtPageSize * 6, env::VirtPageSize * 4, env::MemoryUsage::Execute);
	//for (size_t i = 4; i < 12; i += 2)
	//	memory.munmap(env::VirtPageSize * i, env::VirtPageSize);
	//memory.mmap(env::VirtPageSize * 12, env::VirtPageSize * 4, env::MemoryUsage::Write);
	//memory.mmap(env::VirtPageSize * 20, env::VirtPageSize * 4, env::MemoryUsage::Read);
	//memory.mmap(env::VirtPageSize * 16, env::VirtPageSize * 4, env::MemoryUsage::Execute);
	//memory.mmap(env::VirtPageSize * 6, env::VirtPageSize * 1, env::MemoryUsage::Read);
	//memory.mmap(env::VirtPageSize * 10, env::VirtPageSize * 1, env::MemoryUsage::Execute);
	//memory.mmap(env::VirtPageSize * 8, env::VirtPageSize * 1, env::MemoryUsage::Write);
	//memory.mprotect(env::VirtPageSize * 8, env::VirtPageSize * 6, env::MemoryUsage::Read);
	//memory.munmap(env::VirtPageSize * 15, env::VirtPageSize * 4);
	//memory.mmap(env::VirtPageSize * 0, env::VirtPageSize * 2, env::MemoryUsage::Read);
	//
	//memory.mmap(env::VirtPageSize * 2, env::VirtPageSize * 2, 0);
	//memory.mmap(env::VirtPageSize * 15, env::VirtPageSize * 4, env::MemoryUsage::Execute | env::MemoryUsage::Read);
	//memory.mmap(env::VirtPageSize * 4, env::VirtPageSize * 1, env::MemoryUsage::Write);
	//memory.mprotect(env::VirtPageSize * 1, env::VirtPageSize * 22, env::MemoryUsage::Read);
	//
	//memory.mmap(env::VirtPageSize * 26, env::VirtPageSize * 2, env::MemoryUsage::Read);
	//memory.mmap(env::VirtPageSize * 28, env::VirtPageSize * 2, env::MemoryUsage::Write);
	//memory.mprotect(env::VirtPageSize * 27, env::VirtPageSize * 2, env::MemoryUsage::Write);
	return 0;
}
