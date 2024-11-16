#include "core-builder.h"
#include "../environment/process/process-access.h"

void gen::detail::CoreBuilder::setupCoreBody(wasm::Module& mod, wasm::Memory& physical, wasm::Memory& management) const {
	/* define the bindings passed to the blocks */
	env::detail::ProcessAccess::AddCoreBinding(u8"core", u8"memory_physical");
	env::detail::ProcessAccess::AddCoreBinding(u8"core", u8"memory_management");

	/* setup the actual memory objects */
	uint32_t mpages = env::detail::ProcessAccess::ManagementPages(), ppages = env::detail::ProcessAccess::PhysicalPages();
	physical = mod.memory(u8"memory_physical", wasm::Limit{ ppages }, wasm::Export{});
	management = mod.memory(u8"memory_management", wasm::Limit{ mpages, mpages }, wasm::Export{});
}
void gen::detail::CoreBuilder::setupBlockImports(wasm::Module& mod, wasm::Memory& physical, wasm::Memory& management) const {
	/* setup the memory object imports */
	uint32_t mpages = env::detail::ProcessAccess::ManagementPages(), ppages = env::detail::ProcessAccess::PhysicalPages();
	physical = mod.memory(u8"memory_physical", wasm::Limit{ ppages }, wasm::Import{ u8"core" });
	management = mod.memory(u8"memory_management", wasm::Limit{ mpages, mpages }, wasm::Import{ u8"core" });
}
