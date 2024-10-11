#include "env-process.h"

env::Process::Process(std::u8string_view name, uint32_t cacheSize) : pContext{ name, this }, pMemory{ this, cacheSize } {}

const env::Context& env::Process::context() const {
	return pContext;
}
env::Context& env::Process::context() {
	return pContext;
}
const env::Memory& env::Process::memory() const {
	return pMemory;
}
env::Memory& env::Process::memory() {
	return pMemory;
}
