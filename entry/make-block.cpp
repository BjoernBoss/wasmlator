#include <ustring/ustring.h>
#include <wasgen/wasm.h>
#include <fstream>

#include "../interface/host.h"
#include "../env/env-process.h"

static bool SetupModule(wasm::ModuleInterface* writer) {
	try {
		wasm::Module mod{ writer };

		env::Process* proc = env::Process::Create(u8"test_module", 4, [](env::guest_t addr) {}, [](int32_t) {});
		if (proc == 0)
			return false;
		proc->setupBlockModule(mod);
		proc->release();
	}
	catch (const wasm::Exception& e) {
		str::PrintWLn(L"Exception: ", e.what());
		return false;
	}
	return true;
}

int main(int argc, char** argv) {
	host::SetLogLevel(host::LogLevel::none);

	for (int i = 1; i < argc; ++i) {
		std::string path{ argv[i] };

		if (path.ends_with(".wasm")) {
			writer::BinaryWriter writer;

			/* produce the module as WASM */
			if (!SetupModule(&writer))
				return 1;
			const std::vector<uint8_t>& data = writer.output();

			/* write the file out */
			std::fstream _out{ path, std::ios::out | std::ios::binary };
			_out.write(reinterpret_cast<const char*>(data.data()), data.size());
			str::PrintLn(u8"Generated WASM at: [", path, u8']');
		}
		else if (path.ends_with(".wat")) {
			writer::TextWriter writer;

			/* produce the module as WAT */
			if (!SetupModule(&writer))
				return 1;
			const std::u8string& data = writer.output();

			/* write the file out */
			std::fstream _out{ path, std::ios::out | std::ios::binary };
			_out.write(reinterpret_cast<const char*>(data.data()), data.size());
			str::PrintLn(u8"Generated WAT at: [", path, u8']');
		}
		else {
			str::PrintLn(u8"Invalid usage. Expected .wasm or .wat file paths.");
			return 1;
		}
	}
	return 0;
}
