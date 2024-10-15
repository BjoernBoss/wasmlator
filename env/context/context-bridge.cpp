#include "../env-process.h"
#include "../../interface/interface.h"

void env::bridge::Context::CoreLoaded(uint64_t process, bool succeeded) {
	reinterpret_cast<env::Process*>(process)->context().fCoreLoaded(succeeded);
}
void env::bridge::Context::BlockLoaded(uint64_t process, bool succeeded) {
	reinterpret_cast<env::Process*>(process)->context().fBlockLoaded(succeeded);
}
void env::bridge::Context::Translate(uint64_t process, uint64_t address) {
	reinterpret_cast<env::Process*>(process)->context().fTranslate(address);
}
void env::bridge::Context::Terminated(uint64_t process, int32_t code) {
	reinterpret_cast<env::Process*>(process)->context().fTerminated(code);
}

env::id_t env::bridge::Context::Create(env::Process* process) {
	return ctx_create(reinterpret_cast<uint64_t>(process));
}
bool env::bridge::Context::LoadCore(env::id_t id, const uint8_t* data, size_t size) {
	return (ctx_load_core(id, data, uint32_t(size)) > 0);
}
bool env::bridge::Context::LoadBlock(env::id_t id, const uint8_t* data, size_t size, size_t exports) {
	return (ctx_load_block(id, data, uint32_t(size), uint32_t(exports)) > 0);
}
void env::bridge::Context::Destroy(env::id_t id) {
	ctx_destroy(id);
}
void env::bridge::Context::AddExport(env::id_t id, const std::u8string_view name, env::guest_t address) {
	ctx_add_export(id, name.data(), uint32_t(name.size()), address);
}
