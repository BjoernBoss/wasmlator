#include "../environment.h"

uintptr_t env::detail::ContextAccess::Configure() {
	/* allocate the buffer and return the highest address */
	env::Instance()->context().pBuffer.resize(env::Instance()->system().contextSize());
	return uintptr_t(env::Instance()->context().pBuffer.data() + env::Instance()->context().pBuffer.size());
}
uintptr_t env::detail::ContextAccess::ContextAddress() {
	return uintptr_t(env::Instance()->context().pBuffer.data());
}
size_t env::detail::ContextAccess::ContextSize() {
	return env::Instance()->context().pBuffer.size();
}
