#include "generate.h"

int main(int argc, char** argv) {
	std::string wasmPath{ (argc < 2 ? "" : argv[1]) };
	std::string watPath{ (argc < 3 ? "" : argv[2]) };
	if (argc != 3 || !wasmPath.ends_with(".wasm") || !watPath.ends_with(".wat")) {
		str::PrintLn(u8"Invalid usage. Expected path to wasm-output and path to wat-output.");
		return 1;
	}

	/* generate the wasm and wat files */
	return (glue::Generate(wasmPath, watPath) ? 0 : 1);
}
