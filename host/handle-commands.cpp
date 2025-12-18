/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include <arger/arger.h>
#include "../host/interface.h"
#include "../system/system.h"
#include "../rv64/rv64-cpu.h"

enum class GroupId : uint8_t {
	start, destroy, step, run, until, echo, common, delBreak, delPrint, addBreak,
	inBreak, inPrint, inReg, inInst, inMem8, inMem16, inMem32, inMem64, inEval
};
enum class OptionId : uint8_t {
	debug, environment, depth, trace, log, bind, description
};
static arger::Config Commands{ false,
	arger::GroupName{ L"command" },
	arger::Information{
		L"Expression",
		L"Addresses can be given as expressions of subtraction / addition of constants or registers. An example for such an expression could be \"sp - 0x40 + eax\". All commonly known register names will then be replaced with their corresponding value at evaluation time. Expressions bound to the debugger will be re-evaluated per usage, i.e. always the current register values are used.",
		arger::Reach{ true }
	},
	arger::HelpEntry{ L"help",
		arger::Description{ L"Print this help menu." },
		arger::Reach{ true },
	},
	arger::Group{ L"start", GroupId::start,
		arger::Description{ L"Start a translation process." },
		arger::Positional{ L"binary", arger::Primitive::any, L"Path to the program to be executed." },
		arger::Positional{ L"args", arger::Primitive::any, L"Arguments to be passed to the process." },
		arger::Require::AtLeast(1),
		arger::Option{ L"debug", OptionId::debug,
			arger::Abbreviation{ L'd' },
			arger::Description{ L"Configure the system to startup in debug mode." },
		},
		arger::Option{ L"environment", OptionId::environment,
			arger::Abbreviation{ L'e' },
			arger::Description{ L"Add entry to environment vector (must be of form name=value)." },
			arger::Payload{ L"key-value", arger::Primitive::any },
			arger::Constraint{ [](const arger::Parsed& o) -> std::wstring {
				for (size_t i = 0; i < o.options(OptionId::environment); ++i) {
					std::wstring value = o.option(OptionId::environment, i).value().str();
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
		arger::Option{ L"log", OptionId::log,
			arger::Abbreviation{ L'l' },
			arger::Description{ L"Log all WAT blocks being generated." },
		},
		arger::Option{ L"trace", OptionId::trace,
			arger::Abbreviation{ L't' },
			arger::Require{},
			arger::Payload{L"mode", arger::Enum{
				arger::EnumEntry{ L"inst", gen::TraceType::instruction, L"Trace each executed instruction." },
				arger::EnumEntry{ L"chunk", gen::TraceType::chunk, L"Trace each entered instruction chunk." },
				arger::EnumEntry{ L"block", gen::TraceType::block, L"Trace each entered super-block." },
				arger::EnumEntry{ L"none", gen::TraceType::none, L"Do not perform any tracing." },
			}},
			arger::Default{ L"none" },
			arger::Description{ L"Log the address of all blocks being entered." },
		},
		arger::Option{ L"depth", OptionId::depth,
			arger::Payload{ L"count", arger::Primitive::unum },
			arger::Default{ uint64_t(sys::DefTranslationDepth) },
			arger::Description{ L"Configure the depth to which super-blocks should be crawled and translated at once." },
		},
	},
	arger::Group{ L"destroy", GroupId::destroy,
		arger::Description{ L"Destroy the currently configured translation process." },
	},
	arger::Group{ L"step", GroupId::step,
		arger::Abbreviation{ L's' },
		arger::Description{ L"Step for the number of instructions." },
		arger::Positional{ L"count", arger::Primitive::unum,
			arger::Description{ L"Number of instructions to execute." },
			arger::Default{ 1 },
		},
	},
	arger::Group{ L"run", GroupId::run,
		arger::Abbreviation{ L'r' },
		arger::Description{ L"Continue running until a breakpoint or other exception occurs." },
	},
	arger::Group{ L"until", GroupId::until,
		arger::Abbreviation{ L'u' },
		arger::Description{ L"Continue running until a breakpoint or the given address has been reached." },
		arger::Positional{ L"address", arger::Primitive::any, L"Address to halt at." },
	},
	arger::Group{ L"inspect",
		arger::Abbreviation{ L'i' },
		arger::Description{ L"Inspect the state of the current cpu." },
		arger::Option{ L"bind", OptionId::bind,
			arger::Abbreviation{ L'b' },
			arger::Description{ L"Bind the inspection to be printed on every debug halt." },
		},
		arger::Group{ L"break", GroupId::inBreak,
			arger::Description{ L"Print the set breakpoints." },
		},
		arger::Group{ L"print", GroupId::inPrint,
			arger::Description{ L"Print the bound printings." },
		},
		arger::Group{ L"reg", GroupId::inReg,
			arger::Abbreviation{ L'r' },
			arger::Description{ L"Print the current register bank." },
		},
		arger::Group{ L"insts", GroupId::inInst,
			arger::Abbreviation{ L'i' },
			arger::Description{ L"Print instructions at the address." },
			arger::Require{ 1, 2 },
			arger::Positional{ L"count", arger::Primitive::unum,
				arger::Description{ L"Number of instructions to print." },
				arger::Default{ 1 },
			},
			arger::Positional{ L"address", arger::Primitive::any, L"Instruction to start printing from (defaults to pc)." },
		},
		arger::Group{ L"byte", GroupId::inMem8,
			arger::Abbreviation{ L'b' },
			arger::Description{ L"Print bytes at the address." },
			arger::Positional{ L"address", arger::Primitive::any,  L"Address to print memory from." },
			arger::Positional{ L"count", arger::Primitive::unum, L"Number of bytes to print." },
		},
		arger::Group{ L"word", GroupId::inMem16,
			arger::Abbreviation{ L'w' },
			arger::Description{ L"Print words at the address." },
			arger::Positional{ L"address", arger::Primitive::any, L"Address to print memory from." },
			arger::Positional{ L"count", arger::Primitive::unum, L"Number of words to print." },
		},
		arger::Group{ L"dword", GroupId::inMem32,
			arger::Abbreviation{ L'd' },
			arger::Description{ L"Print dwords at the address." },
			arger::Positional{ L"address", arger::Primitive::any, L"Address to print memory from." },
			arger::Positional{ L"count", arger::Primitive::unum, L"Number of dwords to print." },
		},
		arger::Group{ L"qword", GroupId::inMem64,
			arger::Abbreviation{ L'q' },
			arger::Description{ L"Print qwords at the address." },
			arger::Positional{ L"address", arger::Primitive::any, L"Address to print memory from." },
			arger::Positional{ L"count", arger::Primitive::unum, L"Number of qwords to print." },
		},
		arger::Group{ L"eval", GroupId::inEval,
			arger::Abbreviation{ L'e' },
			arger::Description{ L"Evaluate the value and print it." },
			arger::Positional{ L"value", arger::Primitive::any, L"Value to be evaluated." },
			arger::Positional{ L"hex", arger::Primitive::boolean,
				arger::Description{ L"Print number as hex." },
				arger::Default{ true },
			},
			arger::Option{ L"desc", OptionId::description,
				arger::Payload{ L"text", arger::Primitive::any },
				arger::Description{ L"Add a pre-printed description to the evaluation." },
			},
		},
	},
	arger::Group{ L"echo", GroupId::echo,
		arger::Abbreviation{ L'e' },
		arger::Description{ L"Echo the given message." },
		arger::Positional{ L"msg", arger::Primitive::any, L"Message to be echoed." },
	},
	arger::Group{ L"common", GroupId::common,
		arger::Description{ L"Bind the common debug printing." },
		arger::Require::Any(),
		arger::Positional{ L"sp-name", arger::Primitive::any, L"Name of stack pointer register." },
	},
	arger::Group{ L"del",
		arger::Description{ L"Remove breakpoints or print bindings." },
		arger::Group{ L"break", GroupId::delBreak,
			arger::Abbreviation{ L'b' },
			arger::Description{ L"Remove breakpoint with the given index." },
			arger::Positional{ L"index", arger::Primitive::unum, L"Index of the breakpoint." },
		},
		arger::Group{ L"print", GroupId::delPrint,
			arger::Abbreviation{ L'p' },
			arger::Description{ L"Remove bound print with the given index." },
			arger::Positional{ L"index", arger::Primitive::unum, L"Index of the binding." },
		},
	},
	arger::Group{ L"break", GroupId::addBreak,
		arger::Abbreviation{ L'b' },
		arger::Description{ L"Add a breakpoint to the given address." },
		arger::Positional{ L"address", arger::Primitive::any, L"Address of the breakpoint." },
	},
};

static sys::Debugger* debugger = 0;

static void HandleDebug(const arger::Parsed& out) {
	bool bind = out.flag(OptionId::bind);

	/* handle all of the debug-related arguments */
	switch (out.group<GroupId>()) {
	case GroupId::step:
		debugger->step(out.positional(0).value().unum());
		return;
	case GroupId::run:
		debugger->run();
		return;
	case GroupId::until:
		debugger->until(out.positional(0).value().str<char8_t>());
		return;
	case GroupId::addBreak:
		debugger->addBreak(out.positional(0).value().str<char8_t>());
		return;
	case GroupId::delBreak:
		debugger->dropBreak(out.positional(0).value().unum());
		return;
	case GroupId::delPrint:
		debugger->dropBinding(out.positional(0).value().unum());
		return;
	case GroupId::common:
		if (out.positionals() == 0)
			debugger->setupCommon(std::nullopt);
		else
			debugger->setupCommon(out.positional(0).value().str<char8_t>());
		return;
	case GroupId::inBreak:
		debugger->printBreaks();
		return;
	case GroupId::inPrint:
		debugger->printBindings();
		return;
	case GroupId::inReg:
		debugger->printState(bind);
		return;
	case GroupId::inInst:
		if (out.positionals() == 2)
			debugger->printInstructions(out.positional(1).value().str<char8_t>(), out.positional(0).value().unum(), bind);
		else
			debugger->printInstructions(std::nullopt, out.positional(0).value().unum(), bind);
		return;
	case GroupId::inMem8:
		debugger->printData8(out.positional(0).value().str<char8_t>(), out.positional(1).value().unum() * 1, bind);
		return;
	case GroupId::inMem16:
		debugger->printData16(out.positional(0).value().str<char8_t>(), out.positional(1).value().unum() * 2, bind);
		return;
	case GroupId::inMem32:
		debugger->printData32(out.positional(0).value().str<char8_t>(), out.positional(1).value().unum() * 4, bind);
		return;
	case GroupId::inMem64:
		debugger->printData64(out.positional(0).value().str<char8_t>(), out.positional(1).value().unum() * 8, bind);
		return;
	case GroupId::inEval: {
		debugger->printEval(out.option(OptionId::description).value_or(u8"").str<char8_t>(), out.positional(0).value().str<char8_t>(), out.positional(1).value().boolean(), bind);
		return;
	}
	default:
		return;
	}
}

void HandleCommand(std::u8string_view cmd) {
	util::ConfigureLogging(true);

	/* check if the process has been killed */
	if (env::Instance() == 0)
		debugger = 0;

	/* parse the next command */
	arger::Parsed out;
	try {
		out = arger::Parse(cmd, Commands, 100);
	}
	catch (const arger::PrintMessage& e) {
		util::nullLogger.info(e.what());
		return;
	}
	catch (const arger::ParsingException& e) {
		util::nullLogger.error(e.what(), L' ', arger::HelpHint(L"", Commands));
		return;
	}

	/* handle the create command */
	if (out.group<GroupId>() == GroupId::start) {
		if (env::Instance() != 0) {
			util::nullLogger.error(u8"Process is already loaded.");
			return;
		}

		/* fetch the initial configuration */
		bool debug = out.flag(OptionId::debug);
		sys::RunConfig config{
			.binary = out.positional(0).value().str<char8_t>(),
			.translationDepth = uint32_t(out.option(OptionId::depth).value().unum()),
			.trace = out.option(OptionId::trace).value().id<gen::TraceType>(),
			.logBlocks = out.flag(OptionId::log)
		};

		/* collect the argument vector */
		for (size_t i = 1; i < out.positionals(); ++i)
			config.args.push_back(out.positional(i).value().str<char8_t>());

		/* collect the environment vector */
		size_t envCount = out.options(OptionId::environment);
		for (size_t i = 0; i < envCount; ++i)
			config.envs.push_back(out.option(OptionId::environment, i).value().str<char8_t>());

		/* try to setup the userspace system */
		std::unique_ptr<sys::Cpu> cpu = rv64::Cpu::New();
		if (debug ? (debugger = sys::Userspace::Debug(std::move(cpu), config)) != 0 : sys::Userspace::Create(std::move(cpu), config))
			util::nullLogger.log(u8"Process creation completed");
		else
			util::nullLogger.error(u8"Failed to create process");
		return;
	}

	/* handle the destroy command */
	if (out.group<GroupId>() == GroupId::destroy) {
		debugger = 0;
		if (env::Instance() == 0)
			util::nullLogger.error(u8"No process is currently running.");
		else
			env::Instance()->shutdown();
		return;
	}

	/* handle the debug instruction */
	if (debugger != 0)
		HandleDebug(out);
	else
		util::nullLogger.error(u8"No debugger attached.");
}
