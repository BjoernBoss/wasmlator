#include "env-process.h"

#include "../writer/text-writer.h"

std::u8string env::Process::fCreateMemoryModule() {
	writer::TextWriter writer;
	wasm::Module module{ &writer };

	/* add the core linear memory and page-lookup */
	wasm::Memory memCore = module.memory(wasm::Limit{ env::MinPages }, u8"core_memory", {}, wasm::Export{ u8"core_memory" });
	wasm::Memory memPages = module.memory(wasm::Limit{ env::MinPageMap }, u8"page_memory", {}, wasm::Export{ u8"page_memory" });

	/* add the globals for the page-cache and lookup */
	wasm::Global readMask = module.global(wasm::Type::i64, true, u8"read_mask", {}, wasm::Export{ u8"read_mask" });
	wasm::Global readOffset = module.global(wasm::Type::i32, true, u8"read_offset", {}, wasm::Export{ u8"read_offset" });
	wasm::Global writeMask = module.global(wasm::Type::i64, true, u8"write_mask", {}, wasm::Export{ u8"write_mask" });
	wasm::Global writeOffset = module.global(wasm::Type::i32, true, u8"write_offset", {}, wasm::Export{ u8"write_offset" });
	wasm::Global executeMask = module.global(wasm::Type::i64, true, u8"execute_mask", {}, wasm::Export{ u8"execute_mask" });
	wasm::Global executeOffset = module.global(wasm::Type::i32, true, u8"execute_offset", {}, wasm::Export{ u8"execute_offset" });



	/* close the module and return the overall constructed module */
	module.close();
	return writer.output();
}

env::Process env::Process::Create(std::u8string_view name) {
	env::Process _proc{};
	_proc.pName = std::u8string{ name };
	str::FormatTo(_proc.pLogHeader, u8"Process [{:>12}]: ", _proc.pName);

	/* setup the core modules */
	_proc.fLog(u8"Constructing core memory-module");


	return _proc;
}

