#include "interface.h"

#include <wasgen/wasm.h>
#include <wasgen/writer/binary-writer.h>
#include <wasgen/writer/text-writer.h>

#include "../util/logging.h"
#include "../env/memory/env-memory.h"
#include "../env/context/env-context.h"

struct State {
	env::Context ctx{ u8"test_module" };
	env::Memory memory{ ctx, 4 };
};

void main_startup() {
	util::log(u8"Main: Application startup entered");
	State* state = new State();

	writer::BinaryWriter _writer;
	wasm::Module _module{ &_writer };
	state->memory.setupCoreModule(_module);

	_module.close();
	const std::vector<uint8_t>& data = _writer.output();
	state->ctx.setCore(data.data(), data.size(), [=](bool succeeded) {
		state->memory.mmap(0x0, env::VirtPageSize, env::MemoryUsage::Write);
		for (size_t i = 0; i < 256; ++i)
			state->memory.write<uint8_t>(i, uint8_t(i));

		state->memory.mprotect(0x0, env::VirtPageSize, env::MemoryUsage::Execute);
		for (size_t i = 0; i < 256; ++i)
			state->ctx.log(str::Format<std::u8string>(u8"{:#04x}: {:#02x}", i, state->memory.read<uint8_t>(i)));

		state->memory.munmap(0x0, env::VirtPageSize);

		state->ctx.log(u8"closing down!");
		delete state;
		});
	util::log(u8"Main: Application startup exited");
}
