#include "context-bridge.h"
#include "../../interface/interface.h"

env::id_t env::bridge::Context::Create() {
	return ctx_create();
}
bool env::bridge::Context::SetCore(env::id_t id, const uint8_t* data, size_t size) {
	return (ctx_set_core(id, data, uint32_t(size)) > 0);
}
void env::bridge::Context::Destroy(env::id_t id) {
	ctx_destroy(id);
}
