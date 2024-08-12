#include <wasgen/module.h>
#include <wasgen/wasm.h>
#include <wasgen/text-writer.h>

#include <iostream>

#ifndef EMSCRIPTEN_COMPILATION
int testCall() {
	return 50;
}
#else
extern "C" int testCall();
#endif

struct TStruct {
	int i = 0;
	float f = 0.0f;
};

extern "C" int test(TStruct& t) {
	int x = t.i + int(t.f + 0.5f);

	wasm::Module module;
	module.importFunction({ u8"env", u8"name" }, u8"test", wasm::fn::i32Function);

	int res = testCall();
	return res + x + sizeof(TStruct);
}

#ifndef EMSCRIPTEN_COMPILATION
int main() {
	std::cout << "Hello, World!" << std::endl;

	TextWriter state;
	str::OutLn(wasm::Memory(state, u8"test-memory", wasm::Limit(state, 586)).written.out);

	return 0;
}
#endif
