#include "generate.h"

static bool SetupModule(wasm::ModuleInterface* writer) {
	try {
		glue::State state{ writer };

		glue::SetupHostImports(state);

		glue::InitializeState(state);

		glue::SetupHostBody(state);
		glue::SetupContextFunctions(state);
		glue::SetupMemoryFunctions(state);

		state.module.close();
	}
	catch (const wasm::Exception& e) {
		str::PrintWLn(L"Exception: ", e.what());
		return false;
	}
	return true;
}

int main(int argc, char** argv) {
	std::string wasmPath{ (argc < 2 ? "" : argv[1]) };
	std::string watPath{ (argc < 3 ? "" : argv[2]) };
	if (argc != 3 || !wasmPath.ends_with(".wasm") || !watPath.ends_with(".wat")) {
		str::PrintLn(u8"Invalid usage. Expected path to wasm-output and path to wat-output.");
		return 1;
	}

	{
		writer::BinaryWriter _writer;

		/* produce the module as WASM and write it out */
		if (!SetupModule(&_writer))
			return 1;
		const std::vector<uint8_t>& data = _writer.output();

		std::fstream _out{ wasmPath, std::ios::out | std::ios::binary };
		_out.write(reinterpret_cast<const char*>(data.data()), data.size());
	}

	{
		writer::TextWriter _writer;

		/* produce the module as WAT and write it out */
		if (!SetupModule(&_writer))
			return 1;
		const std::u8string& data = _writer.output();

		std::fstream _out{ watPath, std::ios::out | std::ios::binary };
		_out.write(reinterpret_cast<const char*>(data.data()), data.size());
	}
	return 0;
}
