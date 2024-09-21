#include "interface.h"

#ifdef EMSCRIPTEN_COMPILATION

extern "C" {
	void jsLogImpl(const char8_t* ptr, size_t size);
	void jsFailImpl [[noreturn]] (const char8_t* ptr, size_t size);
}

void _env::u8log(const std::u8string_view& str) {
	jsLogImpl(str.data(), str.size());
}
void _env::u8fail(const std::u8string_view& str) {
	jsFailImpl(str.data(), str.size());
}

#endif
