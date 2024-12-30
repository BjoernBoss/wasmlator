#include "../generate.h"
#include "../environment/process/process-access.h"

void gen::detail::CoreBuilder::setupCoreImports(wasm::Memory& memory) const {
	/* import the main memory */
	uint32_t pages = env::detail::ProcessAccess::MemoryPages();
	memory = gen::Module->memory(u8"memory", wasm::Limit{ pages }, wasm::Transport{ u8"main" });
}
void gen::detail::CoreBuilder::setupCoreBody(wasm::Memory& physical) const {
	/* define the bindings passed to the blocks */
	env::detail::ProcessAccess::AddCoreBinding(u8"core", u8"memory_physical");
	env::detail::ProcessAccess::AddCoreBinding(u8"core", u8"memory");

	/* setup the actual memory object */
	uint32_t pages = env::detail::ProcessAccess::PhysicalPages();
	physical = gen::Module->memory(u8"memory_physical", wasm::Limit{ pages }, wasm::Export{});
}
void gen::detail::CoreBuilder::setupBlockImports(wasm::Memory& physical, wasm::Memory& memory) const {
	/* setup the memory object imports */
	uint32_t mpages = env::detail::ProcessAccess::MemoryPages(), ppages = env::detail::ProcessAccess::PhysicalPages();
	physical = gen::Module->memory(u8"memory_physical", wasm::Limit{ ppages }, wasm::Import{ u8"core" });
	memory = gen::Module->memory(u8"memory", wasm::Limit{ mpages }, wasm::Import{ u8"core" });
}
