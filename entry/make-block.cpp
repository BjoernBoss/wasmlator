#include <ustring/ustring.h>
#include <wasgen/wasm.h>
#include <fstream>

#include "../environment/environment.h"
#include "../generate/generate.h"
#include "null-interface.h"

static bool SetupModule(wasm::ModuleInterface* writer) {
	try {
		wasm::Module mod{ writer };
		gen::Block _block{ mod };
		_block.close();
	}
	catch (const wasm::Exception& e) {
		str::PrintWLn(L"Exception: ", e.what());
		return false;
	}
	return true;
}

int main(int argc, char** argv) {
	host::SetLogLevel(host::LogLevel::none);
	env::SetInstance(std::make_unique<NullSystem>(), 0x1000, 1, 1, false);
	gen::SetInstance(std::make_unique<NullTranslator>(), 1, false, false);

	for (int i = 1; i < argc; ++i) {
		std::string path{ argv[i] };

		if (path.ends_with(".wasm")) {
			wasm::BinaryWriter writer;

			/* produce the module as WASM */
			if (!SetupModule(&writer))
				return 1;
			const std::vector<uint8_t>& data = writer.output();

			/* write the file out */
			std::fstream _out{ path, std::ios::out | std::ios::binary };
			_out.write(reinterpret_cast<const char*>(data.data()), data.size());
		}
		else if (path.ends_with(".wat")) {
			wasm::TextWriter writer;

			/* produce the module as WAT */
			if (!SetupModule(&writer))
				return 1;
			const std::u8string& data = writer.output();

			/* write the file out */
			std::fstream _out{ path, std::ios::out | std::ios::binary };
			_out.write(reinterpret_cast<const char*>(data.data()), data.size());
		}
		else {
			str::PrintLn(u8"Invalid usage. Expected .wasm or .wat file paths.");
			return 1;
		}
		str::PrintLn(L"Created [", path, L']');
	}
	return 0;
}
