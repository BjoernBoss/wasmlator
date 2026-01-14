/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
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
	arger::GroupName{ "command" },
	arger::Information{ "Expression",
		"Addresses can be given as expressions of subtraction / addition of constants or registers. An example for such an expression could be \"sp - 0x40 + eax\". All commonly known register names will then be replaced with their corresponding value at evaluation time. Expressions bound to the debugger will be re-evaluated per usage, i.e. always the current register values are used.",
		arger::Reach{ true }
	},
	arger::HelpEntry{ "help",
		arger::Description{ "Print this help menu." },
		arger::Reach{ true },
	},
	arger::Group{ "start", GroupId::start,
		arger::Description{ "Start a translation process." },
		arger::Positional{ "binary", arger::Primitive::any, "Path to the program to be executed." },
		arger::Positional{ "args", arger::Primitive::any, "Arguments to be passed to the process." },
		arger::Require::AtLeast(1),
		arger::Option{ "debug", OptionId::debug,
			arger::Abbreviation{ 'd' },
			arger::Description{ "Configure the system to startup in debug mode." },
		},
		arger::Option{ "environment", OptionId::environment,
			arger::Abbreviation{ 'e' },
			arger::Description{ "Add entry to environment vector (must be of form name=value)." },
			arger::Payload{ "key-value", arger::Primitive::any },
			arger::Constraint{ [](const arger::Parsed& o) -> std::string {
				for (size_t i = 0; i < o.options(OptionId::environment); ++i) {
					std::string value = o.option(OptionId::environment, i).value().str();
					size_t index = value.size();
					for (size_t i = 0; i < value.size(); ++i) {
						if (value[i] != '=')
							continue;
						if (i == 0 || i + 1 == value.size() || index != value.length()) {
							index = value.size();
							break;
						}
						index = i;
					}
					if (index == value.size())
						return "Malformed environment encountered (expected: name=value).";
				}
				return "";
			}},
			arger::Require::Any(),
		},
		arger::Option{ "log", OptionId::log,
			arger::Abbreviation{ 'l' },
			arger::Description{ "Log all WAT blocks being generated." },
		},
		arger::Option{ "trace", OptionId::trace,
			arger::Abbreviation{ 't' },
			arger::Require{},
			arger::Payload{ "mode", arger::Enum{
				arger::EnumEntry{ "inst", gen::TraceType::instruction, "Trace each executed instruction." },
				arger::EnumEntry{ "chunk", gen::TraceType::chunk, "Trace each entered instruction chunk." },
				arger::EnumEntry{ "block", gen::TraceType::block, "Trace each entered super-block." },
				arger::EnumEntry{ "none", gen::TraceType::none, "Do not perform any tracing." },
			}},
			arger::Default{ "none" },
			arger::Description{ "Log the address of all blocks being entered." },
		},
		arger::Option{ "depth", OptionId::depth,
			arger::Payload{ "count", arger::Primitive::unum },
			arger::Default{ uint64_t(sys::DefTranslationDepth) },
			arger::Description{ "Configure the depth to which super-blocks should be crawled and translated at once." },
		},
	},
	arger::Group{ "destroy", GroupId::destroy,
		arger::Description{ "Destroy the currently configured translation process." },
	},
	arger::Group{ "step", GroupId::step,
		arger::Abbreviation{ 's' },
		arger::Description{ "Step for the number of instructions." },
		arger::Positional{ "count", arger::Primitive::unum,
			arger::Description{ "Number of instructions to execute." },
			arger::Default{ 1 },
		},
	},
	arger::Group{ "run", GroupId::run,
		arger::Abbreviation{ 'r' },
		arger::Description{ "Continue running until a breakpoint or other exception occurs." },
	},
	arger::Group{ "until", GroupId::until,
		arger::Abbreviation{ 'u' },
		arger::Description{ "Continue running until a breakpoint or the given address has been reached." },
		arger::Positional{ "address", arger::Primitive::any, "Address to halt at." },
	},
	arger::Group{ "inspect",
		arger::Abbreviation{ 'i' },
		arger::Description{ "Inspect the state of the current cpu." },
		arger::Option{ "bind", OptionId::bind,
			arger::Abbreviation{ 'b' },
			arger::Description{ "Bind the inspection to be printed on every debug halt." },
		},
		arger::Group{ "break", GroupId::inBreak,
			arger::Description{ "Print the set breakpoints." },
		},
		arger::Group{ "print", GroupId::inPrint,
			arger::Description{ "Print the bound printings." },
		},
		arger::Group{ "reg", GroupId::inReg,
			arger::Abbreviation{ 'r' },
			arger::Description{ "Print the current register bank." },
		},
		arger::Group{ "insts", GroupId::inInst,
			arger::Abbreviation{ 'i' },
			arger::Description{ "Print instructions at the address." },
			arger::Require{ 1, 2 },
			arger::Positional{ "count", arger::Primitive::unum,
				arger::Description{ "Number of instructions to print." },
				arger::Default{ 1 },
			},
			arger::Positional{ "address", arger::Primitive::any, "Instruction to start printing from (defaults to pc)." },
		},
		arger::Group{ "byte", GroupId::inMem8,
			arger::Abbreviation{ 'b' },
			arger::Description{ "Print bytes at the address." },
			arger::Positional{ "address", arger::Primitive::any,  "Address to print memory from." },
			arger::Positional{ "count", arger::Primitive::unum, "Number of bytes to print." },
		},
		arger::Group{ "word", GroupId::inMem16,
			arger::Abbreviation{ 'w' },
			arger::Description{ "Print words at the address." },
			arger::Positional{ "address", arger::Primitive::any, "Address to print memory from." },
			arger::Positional{ "count", arger::Primitive::unum, "Number of words to print." },
		},
		arger::Group{ "dword", GroupId::inMem32,
			arger::Abbreviation{ 'd' },
			arger::Description{ "Print dwords at the address." },
			arger::Positional{ "address", arger::Primitive::any, "Address to print memory from." },
			arger::Positional{ "count", arger::Primitive::unum, "Number of dwords to print." },
		},
		arger::Group{ "qword", GroupId::inMem64,
			arger::Abbreviation{ 'q' },
			arger::Description{ "Print qwords at the address." },
			arger::Positional{ "address", arger::Primitive::any, "Address to print memory from." },
			arger::Positional{ "count", arger::Primitive::unum, "Number of qwords to print." },
		},
		arger::Group{ "eval", GroupId::inEval,
			arger::Abbreviation{ 'e' },
			arger::Description{ "Evaluate the value and print it." },
			arger::Positional{ "value", arger::Primitive::any, "Value to be evaluated." },
			arger::Positional{ "hex", arger::Primitive::boolean,
				arger::Description{ "Print number as hex." },
				arger::Default{ true },
			},
			arger::Option{ "desc", OptionId::description,
				arger::Payload{ "text", arger::Primitive::any },
				arger::Description{ "Add a pre-printed description to the evaluation." },
			},
		},
	},
	arger::Group{ "echo", GroupId::echo,
		arger::Abbreviation{ 'e' },
		arger::Description{ "Echo the given message." },
		arger::Positional{ "msg", arger::Primitive::any, "Message to be echoed." },
	},
	arger::Group{ "common", GroupId::common,
		arger::Description{ "Bind the common debug printing." },
		arger::Require::Any(),
		arger::Positional{ "sp-name", arger::Primitive::any, "Name of stack pointer register." },
	},
	arger::Group{ "del",
		arger::Description{ "Remove breakpoints or print bindings." },
		arger::Group{ "break", GroupId::delBreak,
			arger::Abbreviation{ 'b' },
			arger::Description{ "Remove breakpoint with the given index." },
			arger::Positional{ "index", arger::Primitive::unum, "Index of the breakpoint." },
		},
		arger::Group{ "print", GroupId::delPrint,
			arger::Abbreviation{ 'p' },
			arger::Description{ "Remove bound print with the given index." },
			arger::Positional{ "index", arger::Primitive::unum, "Index of the binding." },
		},
	},
	arger::Group{ "break", GroupId::addBreak,
		arger::Abbreviation{ 'b' },
		arger::Description{ "Add a breakpoint to the given address." },
		arger::Positional{ "address", arger::Primitive::any, "Address of the breakpoint." },
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
		util::nullLogger.error(e.what(), ' ', arger::HelpHint("", Commands));
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
