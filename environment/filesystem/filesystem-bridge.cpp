#include "../environment.h"
#include "../../host/interface.h"

void env::detail::FileSystemBridge::Completed(uint64_t id) {
	env::Instance()->filesystem().fCompleted(id);
}

uint64_t env::detail::FileSystemBridge::QueueTask(const char8_t* task, size_t size) {
	return 0;
}
uint64_t env::detail::FileSystemBridge::ReadResult() {
	return 0;
}
bool env::detail::FileSystemBridge::ReadSuccess() {
	return 0;
}
size_t env::detail::FileSystemBridge::ReadStats() {
	return 0;
}
void env::detail::FileSystemBridge::SelectStat(size_t index) {

}
uint64_t env::detail::FileSystemBridge::ReadStatKey(const char8_t* name, size_t size) {
	return 0;
}

void env::detail::FileSystemBridge::FreeStatKey(char8_t* data) {
	free_allocated(data);
}
