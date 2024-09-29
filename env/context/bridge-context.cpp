#include "bridge-context.h"

#ifdef EMSCRIPTEN_COMPILATION

extern "C" {
	uint32_t jsContextCreate();
	void jsContextSetCore(uint32_t id, const uint8_t* data, uint32_t size);
	void jsContextDestroy(uint32_t id);
}

env::id_t env::bridge::Context::Create() {
	return jsContextCreate();
}
void env::bridge::Context::SetCore(env::id_t id, const uint8_t* data, size_t size) {
	jsContextSetCore(id, data, uint32_t(size));
}
void env::bridge::Context::Destroy(env::id_t id) {
	jsContextDestroy(id);
}

#else

// #error Currently not supported

env::id_t env::bridge::Context::Create() {
	return {};
}
void env::bridge::Context::SetCore(env::id_t id, const uint8_t* data, size_t size) {}
void env::bridge::Context::Destroy(env::id_t id) {}

#endif
