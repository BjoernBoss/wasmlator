#include "context-bridge.h"
#include "../../interface/interface.h"

void env::bridge::Context::CoreLoaded(uint64_t self, bool succeeded) {
	reinterpret_cast<env::Context*>(self)->fCoreLoaded(succeeded);
}

env::id_t env::bridge::Context::Create(env::Context* self) {
	return ctx_create(reinterpret_cast<uint64_t>(self));
}
bool env::bridge::Context::SetCore(env::id_t id, const uint8_t* data, size_t size) {
	return (ctx_set_core(id, data, uint32_t(size)) > 0);
}
void env::bridge::Context::Destroy(env::id_t id) {
	ctx_destroy(id);
}
