#include "context-bridge.h"
#include "../env-process.h"
#include "../../interface/interface.h"

void env::bridge::Context::CoreLoaded(uint64_t process, bool succeeded) {
	reinterpret_cast<env::Process*>(process)->context().fCoreLoaded(succeeded);
}
void env::bridge::Context::Translate(uint64_t process, uint64_t address) {
	reinterpret_cast<env::Process*>(process)->context().fTranslate(address);
}

env::id_t env::bridge::Context::Create(env::Process* process) {
	return ctx_create(reinterpret_cast<uint64_t>(process));
}
bool env::bridge::Context::SetCore(env::id_t id, const uint8_t* data, size_t size) {
	return (ctx_set_core(id, data, uint32_t(size)) > 0);
}
void env::bridge::Context::Destroy(env::id_t id) {
	ctx_destroy(id);
}
