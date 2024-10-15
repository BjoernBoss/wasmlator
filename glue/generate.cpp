#include "generate.h"

static bool SetupModule(wasm::ModuleInterface* writer) {
	try {
		glue::State state{ writer };

		glue::SetupHostImports(state);

		glue::InitializeState(state);

		glue::SetupHostBody(state);
		glue::SetupContextFunctions(state);
		glue::SetupMapFunctions(state);
		glue::SetupMemoryFunctions(state);

		state.module.close();
	}
	catch (const wasm::Exception& e) {
		str::PrintWLn(L"Exception: ", e.what());
		return false;
	}
	return true;
}

bool glue::Generate(const std::string& wasmPath, const std::string& watPath) {
	{
		/* produce the module as WASM */
		writer::BinaryWriter _writer;
		if (!SetupModule(&_writer))
			return false;
		const std::vector<uint8_t>& data = _writer.output();

		/* write the file out */
		std::fstream _out{ wasmPath, std::ios::out | std::ios::binary };
		_out.write(reinterpret_cast<const char*>(data.data()), data.size());
		str::PrintLn(u8"Generated WASM at: [", wasmPath, u8']');
	}

	{
		/* produce the module as WAT */
		writer::TextWriter _writer;
		if (!SetupModule(&_writer))
			return false;
		const std::u8string& data = _writer.output();

		/* write the file out */
		std::fstream _out{ watPath, std::ios::out | std::ios::binary };
		_out.write(reinterpret_cast<const char*>(data.data()), data.size());
		str::PrintLn(u8"Generated WAT at: [", wasmPath, u8']');
	}

	return true;
}
