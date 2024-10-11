#include "interface.h"

#include <wasgen/wasm.h>
#include <wasgen/writer/binary-writer.h>
#include <wasgen/writer/text-writer.h>

#include "../util/logging.h"
#include "../env/env-process.h"


void main_startup() {
	util::log(u8"Main: Application startup entered");
	env::Process* process = new env::Process{ u8"test_module", 4 };

	process->context().create();

	writer::BinaryWriter _writer;
	wasm::Module _module{ &_writer };
	process->setupCoreModule(_module);

	_module.close();
	const std::vector<uint8_t>& data = _writer.output();
	process->context().setCore(data.data(), data.size(), [=](bool succeeded) {
		process->memory().mmap(0x0, env::VirtPageSize, env::MemoryUsage::Write);
		for (size_t i = 0; i < 256; ++i)
			process->memory().write<uint8_t>(i, uint8_t(i));

		process->memory().mprotect(0x0, env::VirtPageSize, env::MemoryUsage::Execute);
		for (size_t i = 0; i < 256; ++i)
			process->log(str::Format<std::u8string>(u8"{:#06x}: {:02x}", i, process->memory().execute<uint8_t>(i)));

		process->memory().munmap(0x0, env::VirtPageSize);

		process->log(u8"closing down!");
		delete process;
		});
	util::log(u8"Main: Application startup exited");
}
