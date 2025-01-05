#include <arger/arger.h>
#include "../interface/interface.h"
#include "../interface/host.h"
#include "../environment/environment.h"
#include "../system/sys-primitive.h"
#include "../system/rv64/rv64-cpu.h"

static arger::Config Commands{
	arger::GroupName{ L"command" },
	arger::Option{ L"depth",
		arger::Abbreviation{ L'd' },
		arger::Payload{ L"count", arger::Primitive::unum },
		arger::Description{ L"Depth to recurively visit and translate addresses when translating a block." },
	},
	arger::Option{ L"step",
		arger::Abbreviation{ L's' },
		arger::Description{ L"Only perform single instruction execution steps (necessary to allow proper debugging)." },
	},
	arger::Option{ L"print",
		arger::Abbreviation{ L'p' },
		arger::Description{ L"Print every translated block as WAT." },
	},
	arger::Group{L"setup", L"",
		arger::Description{ L"Setup a translation process." },
		arger::Use{ L"depth", L"step" },
	},
	arger::Group{ L"destroy", L"",
		arger::Description{ L"Destroy the currently configured translation process." },
	},
	arger::Group{ L"help", L"",
		arger::HelpFlag{},
		arger::Description{ L"Print this help menu." },
	},
};

static host::Logger logger{ u8"" };

void StartupProcess() {
	logger.log(u8"StartupProcess");
}

static bool DispatchCommand(std::u8string_view cmd) {
	arger::Parsed out;
	try {
		out = arger::Menu(cmd, Commands);
	}
	catch (const arger::PrintMessage& e) {
		logger.info(u8"\n\n", e.what());
		return true;
	}
	catch (const arger::ParsingException& e) {
		logger.fatal(e.what(), u8" Use 'help' for more information.");
		return true;
	}

	if (out.groupId() == L"setup") {
		if (env::Instance() != 0) {
			logger.fatal(u8"A process is already running.");
			return true;
		}

		logger.log(u8"Setting up system: [primitive] with cpu: [rv64]");
		sys::Primitive::Create(rv64::Cpu::New(),
			{ u8"/this/path", u8"abc-def" },
			{ u8"abc=def", u8"ghi=jkl" },
			true);
		logger.log(u8"Process creation completed");
		return true;
	}
	else if (out.groupId() == L"destroy") {
		if (env::Instance() == 0) {
			logger.fatal(u8"No process is currently running.");
			return true;
		}
		env::Instance()->system()->shutdown();
		return true;
	}

	return false;
}

void HandleCommand(std::u8string_view cmd) {
	if (cmd.empty() || !DispatchCommand(cmd))
		logger.warn(u8"Unknown command [", cmd, u8"] received");
}
