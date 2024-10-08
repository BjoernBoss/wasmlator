#include "generate.h"

static void SetupModule(wasm::ModuleInterface* writer) {
	glue::State state{ writer };

	glue::SetupHostImports(state);

	glue::InitializeState(state);

	glue::SetupHostBody(state);
	glue::SetupContextFunctions(state);
	glue::SetupMemoryFunctions(state);
}

int main() {
	{
		writer::BinaryWriter _writer;

		/* produce the module as WASM and write it out */
		SetupModule(&_writer);
		const std::vector<uint8_t>& data = _writer.output();

		std::fstream _out{ "./glue-module.wasm", std::ios::out | std::ios::binary };
		_out.write(reinterpret_cast<const char*>(data.data()), data.size());
	}

	{
		writer::TextWriter _writer;

		/* produce the module as WAT and write it out */
		SetupModule(&_writer);
		const std::u8string& data = _writer.output();

		std::fstream _out{ "./glue-module.wat", std::ios::out | std::ios::binary };
		_out.write(reinterpret_cast<const char*>(data.data()), data.size());
	}
	return 0;
}
