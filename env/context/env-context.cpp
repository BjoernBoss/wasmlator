#include "env-context.h"
#include "context-bridge.h"

env::Context::Context(std::u8string_view name) : pName{ name }, pSelfName{ u8"core" } {
	str::FormatTo(pLogHeader, u8"Context [{:>12}]: ", pName);

	fLog(u8"Creating new context");
	pId = bridge::Context::Create();
	fLog(u8"Context created with id [", pId, u8']');
}
env::Context::~Context() {
	fLog(u8"Destroying context [", pId, u8']');
	bridge::Context::Destroy(pId);
}

bool env::Context::setCore(const uint8_t* data, size_t size) {
	fLog(u8"Loading new core");
	return bridge::Context::SetCore(pId, data, size);
}
const std::u8string& env::Context::name() const {
	return pName;
}
const std::u8string& env::Context::selfName() const {
	return pSelfName;
}
env::id_t env::Context::id() const {
	return pId;
}
