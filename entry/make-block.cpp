#include <ustring/ustring.h>
#include <wasgen/wasm.h>
#include <fstream>

#include "../interface/host.h"
#include "../env/env-process.h"
#include "../trans/trans-translator.h"

struct TransInterface final : public trans::TranslationInterface {
	void blockStarted() override {}
	void blockCompleted() override {}
	trans::Instruction fetch(env::guest_t addr) override {
		return trans::Instruction{ 0, 0, 0, 0, trans::InstType::invalid };
	}
	void produce(const trans::Writer& writer, const trans::Instruction* data, size_t count) override {}
};

static bool SetupModule(wasm::ModuleInterface* writer) {
	try {
		wasm::Module mod{ writer };
		TransInterface _interface;
		trans::Translator _translator{ mod, &_interface, 4 };
	}
	catch (const wasm::Exception& e) {
		str::PrintWLn(L"Exception: ", e.what());
		return false;
	}
	return true;
}

int main(int argc, char** argv) {
	host::SetLogLevel(host::LogLevel::none);
	env::Process::Create(4, 0);

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
