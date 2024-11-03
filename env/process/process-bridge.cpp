#include "../env-process.h"
#include "../interface/interface.h"

void env::bridge::Process::CoreLoaded(uint64_t process, bool succeeded) {
	reinterpret_cast<env::Process*>(process)->fCoreLoaded(succeeded);
}
void env::bridge::Process::BlockLoaded(uint64_t process, bool succeeded) {
	reinterpret_cast<env::Process*>(process)->fBlockLoaded(succeeded);
}

env::id_t env::bridge::Process::Create(env::Process* process) {
	return proc_create(reinterpret_cast<uint64_t>(process));
}
bool env::bridge::Process::LoadCore(env::id_t id, const uint8_t* data, size_t size) {
	return (proc_load_core(id, data, uint32_t(size)) > 0);
}
bool env::bridge::Process::LoadBlock(env::id_t id, const uint8_t* data, size_t size, size_t exports) {
	return (proc_load_block(id, data, uint32_t(size), uint32_t(exports)) > 0);
}
void env::bridge::Process::Destroy(env::id_t id) {
	proc_destroy(id);
}
void env::bridge::Process::AddExport(env::id_t id, const std::u8string_view name, env::guest_t address) {
	proc_add_export(id, name.data(), uint32_t(name.size()), address);
}
