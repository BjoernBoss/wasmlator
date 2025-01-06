#include <arger/arger.h>
#include "../interface/interface.h"
#include "../interface/logger.h"
#include "../environment/environment.h"
#include "../system/system.h"

static arger::Config Commands{
	arger::GroupName{ L"command" },
	arger::Option{ L"help",
		arger::Abbreviation{ L'h' },
		arger::HelpFlag{},
		arger::Description{ L"Print this help menu." },
	},
	arger::Group{ L"setup", L"",
		arger::Description{ L"Setup a translation process." },
		arger::Positional{ L"args",
			arger::Primitive::any,
			L"Arguments to be passed to the process."
		},
		arger::Require::Any(),
		arger::Option{ L"debug",
			arger::Abbreviation{ L'd' },
			arger::Description{ L"Configure the system to startup in debug mode." },
		},
		arger::Option{ L"environment",
			arger::Abbreviation{ L'e' },
			arger::Description{ L"Add entry to environment vector (must be of form name=value)." },
			arger::Payload{ L"key-value", arger::Primitive::any },
			arger::Constraint{ [](const arger::Parsed& o) -> std::wstring {
				for (size_t i = 0; i < o.options(L"environment"); ++i) {
					std::wstring value = o.option(L"environment", i).value().str();
					size_t index = value.size();
					for (size_t i = 0; i < value.size(); ++i) {
						if (value[i] != L'=')
							continue;
						if (i == 0 || i + 1 == value.size() || index != value.length()) {
							index = value.size();
							break;
						}
						index = i;
					}
					if (index == value.size())
						return L"Malformed environment encountered (expected: name=value).";
				}
				return L"";
			}},
			arger::Require::Any(),
		},
		arger::Option{ L"system",
			arger::Abbreviation{ L's' },
			arger::Description{ L"Configure the system to be used." },
			arger::Payload{ L"name",
				arger::Enum{
					{ L"primitive", L"Setup a primitive system, which constructs a userspace execution." },
				},
				arger::Value{ L"primitive" }
			},
		},
		arger::Option{ L"cpu",
			arger::Abbreviation{ L'c' },
			arger::Description{ L"Configure the cpu to be used." },
			arger::Payload{ L"name",
				arger::Enum{
					{ L"rv64", L"RISC-V 64bit cpu." },
				},
				arger::Value{ L"rv64" }
			},
		},
	},
	arger::Group{ L"destroy", L"",
		arger::Description{ L"Destroy the currently configured translation process." },
	},
	arger::Group{ L"help", L"",
		arger::HelpFlag{},
		arger::Description{ L"Print this help menu." },
	},
	arger::Group{ L"step", L"",
		arger::Abbreviation{ L's' },
		arger::Description{ L"Step for the number of instructions." },
		arger::Positional{ L"count", arger::Primitive::unum, L"Number of instructions to execute.", arger::Value{ 1 } },
	},
};

static host::Logger logger{ u8"" };

void HandleCommand(std::u8string_view cmd) {
	logger.info(u8"Handling: ", cmd);
	arger::Parsed out;

	/* parse the next command */
	try {
		out = arger::Menu(cmd, Commands, 160);
	}
	catch (const arger::PrintMessage& e) {
		logger.info(e.what());
		return;
	}
	catch (const arger::ParsingException& e) {
		logger.error(e.what(), u8" Use 'help' for more information.");
		return;
	}

	/* handle the debug instructions */
	if (out.groupId() == L"step") {
		if (sys::Instance() == 0) {
			logger.error(u8"No debugger attached.");
			return;
		}
		sys::Instance()->step(out.positional(0).value().unum());
		return;
	}

	/* handle the create command */
	if (out.groupId() == L"setup") {
		if (env::Instance() != 0) {
			logger.error(u8"Process is already loaded.");
			return;
		}

		/* system can currently only be 'primitive' and cpu can only be 'rv64' */
		std::wstring system = out.option(L"system").value().str();
		std::wstring cpu = out.option(L"cpu").value().str();
		bool debug = out.flag(L"debug");
		logger.log(u8"Setting up system: [", system, u8"] with cpu: [", cpu, u8"] in configuration [", (debug ? u8"debug]" : u8"normal]"));

		/* collect the argument vector */
		std::vector<std::u8string> args;
		for (size_t i = 0; i < out.positionals(); ++i)
			args.push_back(str::u8::To(out.positional(i).value().str()));

		/* collect the environment vector */
		std::vector<std::u8string> envs;
		size_t envCount = out.options(L"environment");
		for (size_t i = 0; i < envCount; ++i)
			envs.push_back(str::u8::To(out.option(L"environment", i).value().str()));

		/* try to setup the primitive system */
		if (!sys::Primitive::Create(rv64::Cpu::New(), args, envs, debug))
			logger.error(u8"Failed to create process");
		else
			logger.log(u8"Process creation completed");
		return;
	}

	/* handle the destroy command */
	if (out.groupId() == L"destroy") {
		if (env::Instance() == 0)
			logger.error(u8"No process is currently running.");
		else
			env::Instance()->shutdown();
		return;
	}
}
