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
	wasm::Variable count = sink.local(wasm::Type::i32, u8"count");

	/* check if should terminate */
	sink[wasm::inst::U64::Const(0)];
	sink[wasm::inst::Local::Set(address)];
	proc->memory().makeRead(address, state, 0, env::MemoryType::i32);
	sink[wasm::inst::Local::Tee(count)];
	sink[wasm::inst::U32::Const(2048)];
	sink[wasm::inst::U32::GreaterEqual()];
	{
		wasm::IfThen _if{ sink };
		wasm::Variable result = sink.local(wasm::Type::i32, u8"code");
		sink[wasm::inst::U32::Const(123)];
		sink[wasm::inst::Local::Set(result)];
		proc->context().makeExit(result, state);
	}

	/* increase the counter */
	sink[wasm::inst::Local::Get(count)];
	sink[wasm::inst::U32::Const(1)];
	sink[wasm::inst::U32::Add()];
	sink[wasm::inst::Local::Set(count)];
	proc->memory().makeWrite(address, count, state, 1, env::MemoryType::i32);

	/* go to random address */
	sink[wasm::inst::U64::Const(rand() % 1000)];
	sink[wasm::inst::Local::Set(address)];
	proc->mapping().makeGoto(address, state);
	return name;
}

static void translateAddress(env::Process* process, env::guest_t addr) {
	writer::BinaryWriter _writer;
	wasm::Module _module{ &_writer };
	env::ModuleState state = process->setupBlockModule(_module);

	std::u8string name = writeNullFunction(_module, process, state, addr);
	_module.close();


	const std::vector<uint8_t>& data = _writer.output();
	process->loadBlock(data.data(), data.size(), { { name, addr } }, [=](bool succeeded) {
		if (succeeded)
			process->mapping().execute(addr);
		});
}

void main_startup() {
	static struct State {
		env::Process* process = 0;
	} _state;

	util::log(u8"Main: Application startup entered");
	_state.process = env::Process::Create(u8"test_module", 4, [&](env::guest_t addr) { translateAddress(_state.process, addr); },
		[&](int32_t terminated) {
			util::log(u8"Process terminated with [", terminated, u8"]");
			_state.process->release();
		});

	if (_state.process == 0)
		return;

	writer::BinaryWriter _writer;
	wasm::Module _module{ &_writer };
	_state.process->setupCoreModule(_module);

	_module.close();
	const std::vector<uint8_t>& data = _writer.output();
	_state.process->loadCore(data.data(), data.size(), [=](bool succeeded) {
		if (!succeeded) {
			_state.process->log(u8"failed!");
			_state.process->release();
			return;
		}

		_state.process->memory().mmap(0x0, env::VirtPageSize, env::MemoryUsage::Read | env::MemoryUsage::Write);
		_state.process->mapping().execute(0x0123);
		});
	util::log(u8"Main: Application startup exited");
}
