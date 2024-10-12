#include "interface.h"

#include <wasgen/wasm.h>
#include <wasgen/writer/binary-writer.h>
#include <wasgen/writer/text-writer.h>

#include "../util/logging.h"
#include "../env/env-process.h"


void main_startup() {
	util::log(u8"Main: Application startup entered");
	env::Process* process = env::Process::Create(u8"test_module", [](env::guest_t addr) {
		});

	if (process == 0)
		return;

	writer::BinaryWriter _writer;
	wasm::Module _module{ &_writer };
	process->setupCoreModule(_module, 4);

	_module.close();
	const std::vector<uint8_t>& data = _writer.output();
	process->initialize(data.data(), data.size(), [=](bool succeeded) {
		if (!succeeded) {
			process->log(u8"failed!");
			process->release();
			return;
		}

		process->memory().mmap(0x0, env::VirtPageSize, env::MemoryUsage::Write);
		for (size_t i = 0; i < 256; ++i)
			process->memory().write<uint8_t>(i, uint8_t(i));

		process->memory().mprotect(0x0, env::VirtPageSize, env::MemoryUsage::Execute);
		for (size_t i = 0; i < 256; ++i)
			process->log(str::Format<std::u8string>(u8"{:#06x}: {:02x}", i, process->memory().code<uint8_t>(i)));

		process->memory().munmap(0x0, env::VirtPageSize);

		process->blocks().execute(0x0123);

		process->log(u8"closing down!");
		process->release();
		});
	util::log(u8"Main: Application startup exited");
}
