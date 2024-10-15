#include "interface.h"

#include <wasgen/wasm.h>
#include <wasgen/writer/binary-writer.h>
#include <wasgen/writer/text-writer.h>

#include "../util/logging.h"
#include "../env/env-process.h"

#include <stdlib.h>

static std::u8string writeNullFunction(wasm::Module& mod, env::Process* proc, env::ModuleState& state, env::guest_t addr) {
	std::u8string name = str::Format<std::u8string>(u8"func_{:#018x}", addr);
	wasm::Sink sink{ mod.function(name, {}, {}, wasm::Export{}) };
	wasm::Variable address = sink.local(wasm::Type::i64, u8"address");
	sink[wasm::inst::U64::Const(rand() % 1000)];
	sink[wasm::inst::Local::Set(address)];
	proc->mapping().makeGoto(address, state);
	return name;
}

void main_startup() {
	static struct State {
		env::Process* process = 0;
	} _state;

	util::log(u8"Main: Application startup entered");
	_state.process = env::Process::Create(u8"test_module", 4, [&](env::guest_t addr) {
		writer::BinaryWriter _writer;
		wasm::Module _module{ &_writer };
		env::ModuleState state = _state.process->setupBlockModule(_module);

		std::u8string name = writeNullFunction(_module, _state.process, state, addr);
		_module.close();


		const std::vector<uint8_t>& data = _writer.output();
		_state.process->context().loadBlock(data.data(), data.size(), { { name, addr } }, [=](bool succeeded) {
			if (succeeded)
				_state.process->mapping().execute(addr);
			});
		});

	if (_state.process == 0)
		return;

	writer::BinaryWriter _writer;
	wasm::Module _module{ &_writer };
	_state.process->setupCoreModule(_module);

	_module.close();
	const std::vector<uint8_t>& data = _writer.output();
	_state.process->context().loadCore(data.data(), data.size(), [=](bool succeeded) {
		if (!succeeded) {
			_state.process->log(u8"failed!");
			_state.process->release();
			return;
		}

		_state.process->memory().mmap(0x0, env::VirtPageSize, env::MemoryUsage::Write);
		for (size_t i = 0; i < 256; ++i)
			_state.process->memory().write<uint8_t>(i, uint8_t(i));

		_state.process->memory().mprotect(0x0, env::VirtPageSize, env::MemoryUsage::Execute);
		for (size_t i = 0; i < 256; ++i)
			_state.process->log(str::Format<std::u8string>(u8"{:#06x}: {:02x}", i, _state.process->memory().code<uint8_t>(i)));

		_state.process->memory().munmap(0x0, env::VirtPageSize);

		_state.process->mapping().execute(0x0123);
		});
	util::log(u8"Main: Application startup exited");
}
