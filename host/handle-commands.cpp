#include <arger/arger.h>
#include "../host/interface.h"
#include "../system/system.h"
#include "../rv64/rv64-cpu.h"

static arger::Config Commands{
	arger::GroupName{ L"command" },
	arger::Help{
		L"Expression",
		L"Addresses can be given as expressions of subtraction / addition of constants or registers.",
	},
	arger::Group{ L"start", L"",
		arger::Description{ L"Start a translation process." },
		arger::Positional{ L"binary",
			arger::Primitive::any,
			L"Path to the program to be executed."
		},
		arger::Positional{ L"args",
			arger::Primitive::any,
			L"Arguments to be passed to the process."
		},
		arger::Require::AtLeast(1),
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
		arger::Option{ L"log",
			arger::Abbreviation{ L'l' },
			arger::Description{ L"Log all WAT blocks being generated." },
		},
		arger::Option{ L"trace",
			arger::Abbreviation{ L't' },
			arger::Require{},
			arger::Payload{L"mode", arger::Enum{
				{ L"inst", L"Trace each executed instruction." },
				{ L"chunk", L"Trace each entered instruction chunk." },
				{ L"block", L"Trace each entered super-block." },
				{ L"none", L"Do not perform any tracing." },
			},
			arger::Value{ L"none" } },
			arger::Description{ L"Log the address of all blocks being entered." },
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
	arger::Group{ L"run", L"",
		arger::Abbreviation{ L'r' },
		arger::Description{ L"Continue running until a breakpoint or other exception occurs." },
	},
	arger::Group{ L"until", L"",
		arger::Abbreviation{ L'u' },
		arger::Description{ L"Continue running until a breakpoint or the given address has been reached." },
		arger::Positional{ L"address", arger::Primitive::any, L"Address to halt at." },
	},
	arger::Group{ L"inspect", L"",
		arger::Abbreviation{ L'i' },
		arger::Description{ L"Inspect the state of the current cpu." },
		arger::Option{ L"bind",
			arger::Abbreviation{ L'b' },
			arger::Description{ L"Bind the inspection to be printed on every debug halt." },
		},
		arger::Group{ L"break", L"in-break",
			arger::Description{ L"Print the set breakpoints." },
		},
		arger::Group{ L"print", L"in-print",
			arger::Description{ L"Print the bound printings." },
		},
		arger::Group{ L"reg", L"in-reg",
			arger::Abbreviation{ L'r' },
			arger::Description{ L"Print the current register bank." },
		},
		arger::Group{ L"insts", L"in-inst",
			arger::Abbreviation{ L'i' },
			arger::Description{ L"Print instructions ad the address." },
			arger::Require::AtLeast(1),
			arger::Positional{ L"count", arger::Primitive::unum, L"Number of instructions to print.", arger::Value{ 1 } },
			arger::Positional{ L"address", arger::Primitive::any, L"Instruction to start printing from (defaults to pc)." },
		},
		arger::Group{ L"byte", L"in-mem8",
			arger::Abbreviation{ L'b' },
			arger::Description{ L"Print bytes at the address." },
			arger::Positional{ L"address", arger::Primitive::any, L"Address to print memory from." },
			arger::Positional{ L"count", arger::Primitive::unum, L"Number of bytes to print." },
		},
		arger::Group{ L"word", L"in-mem16",
			arger::Abbreviation{ L'w' },
			arger::Description{ L"Print words at the address." },
			arger::Positional{ L"address", arger::Primitive::any, L"Address to print memory from." },
			arger::Positional{ L"count", arger::Primitive::unum, L"Number of words to print." },
		},
		arger::Group{ L"dword", L"in-mem32",
			arger::Abbreviation{ L'd' },
			arger::Description{ L"Print dwords at the address." },
			arger::Positional{ L"address", arger::Primitive::any, L"Address to print memory from." },
			arger::Positional{ L"count", arger::Primitive::unum, L"Number of dwords to print." },
		},
		arger::Group{ L"qword", L"in-mem64",
			arger::Abbreviation{ L'q' },
			arger::Description{ L"Print qwords at the address." },
			arger::Positional{ L"address", arger::Primitive::any, L"Address to print memory from." },
			arger::Positional{ L"count", arger::Primitive::unum, L"Number of qwords to print." },
		},
		arger::Group{ L"eval", L"in-eval",
			arger::Abbreviation{ L'e' },
			arger::Description{ L"Evaluate the value and print it." },
			arger::Positional{ L"value", arger::Primitive::any, L"Value to be evaluated." },
			arger::Positional{ L"hex", arger::Primitive::boolean, L"Print number as hex.", arger::Value{ true } },
			arger::Option{ L"desc",
				arger::Payload{ L"text", arger::Primitive::any },
				arger::Description{ L"Add a pre-printed description to the evaluation." },
			},
		},
	},
	arger::Group{ L"echo", L"",
		arger::Abbreviation{ L'e' },
		arger::Description{ L"Echo the given message." },
		arger::Positional{ L"msg", arger::Primitive::any, L"Message to be echoed." },
	},
	arger::Group{ L"common", L"",
		arger::Description{ L"Bind the common debug printing." },
		arger::Require::Any(),
		arger::Positional{ L"sp-name", arger::Primitive::any, L"Name of stack pointer register." },
	},
	arger::Group{ L"del", L"",
		arger::Description{ L"Remove breakpoints or print bindings." },
		arger::Group{ L"break", L"break-remove",
			arger::Abbreviation{ L'b' },
			arger::Description{ L"Remove breakpoint with the given index." },
			arger::Positional{ L"index", arger::Primitive::unum, L"Index of the breakpoint." },
		},
		arger::Group{ L"print", L"print-remove",
			arger::Abbreviation{ L'p' },
			arger::Description{ L"Remove bound print with the given index." },
			arger::Positional{ L"index", arger::Primitive::unum, L"Index of the binding." },
		},
	},
	arger::Group{ L"break", L"break-add",
		arger::Abbreviation{ L'b' },
		arger::Description{ L"Add a breakpoint to the given address." },
		arger::Positional{ L"address", arger::Primitive::any, L"Address of the breakpoint." },
	},
};

static sys::Debugger* debugger = 0;

static void HandleDebug(const arger::Parsed& out) {
	/* check if the program should take a number of steps */
	if (out.groupId() == L"step") {
		debugger->step(out.positional(0).value().unum());
		return;
	}

	/* check if the program should just run */
	if (out.groupId() == L"run") {
		debugger->run();
		return;
	}

	/* check if the program should run until an address has been reached */
	if (out.groupId() == L"until") {
		debugger->until(str::u8::To(out.positional(0).value().str()));
		return;
	}

	/* check if breakpoints should be added */
	if (out.groupId() == L"break-add") {
		debugger->addBreak(str::u8::To(out.positional(0).value().str()));
		return;
	}

	/* check if breakpoints or bindings should be removed */
	if (out.groupId() == L"break-remove") {
		debugger->dropBreak(out.positional(0).value().unum());
		return;
	}
	if (out.groupId() == L"bind-remove") {
		debugger->dropBinding(out.positional(0).value().unum());
		return;
	}

	/* check if the common bindings should be set up */
	if (out.groupId() == L"common") {
		if (out.positionals() == 0)
			debugger->setupCommon(std::nullopt);
		else
			debugger->setupCommon(str::u8::To(out.positional(0).value().str()));
		return;
	}

	/* check if the debug-state should be inspected */
	if (out.groupId() == L"in-break") {
		debugger->printBreaks();
		return;
	}
	if (out.groupId() == L"in-print") {
		debugger->printBindings();
		return;
	}

	/* check if the state should be inspected and potentially bound */
	bool bind = out.flag(L"bind");
	if (out.groupId() == L"in-reg") {
		debugger->printState(bind);
		return;
	}
	if (out.groupId() == L"in-inst") {
		if (out.positionals() == 2)
			debugger->printInstructions(str::u8::To(out.positional(1).value().str()), out.positional(0).value().unum(), bind);
		else
			debugger->printInstructions(std::nullopt, out.positional(0).value().unum(), bind);
		return;
	}
	if (out.groupId() == L"in-mem8") {
		debugger->printData8(str::u8::To(out.positional(0).value().str()), out.positional(1).value().unum() * 1, bind);
		return;
	}
	if (out.groupId() == L"in-mem16") {
		debugger->printData16(str::u8::To(out.positional(0).value().str()), out.positional(1).value().unum() * 2, bind);
		return;
	}
	if (out.groupId() == L"in-mem32") {
		debugger->printData32(str::u8::To(out.positional(0).value().str()), out.positional(1).value().unum() * 4, bind);
		return;
	}
	if (out.groupId() == L"in-mem64") {
		debugger->printData64(str::u8::To(out.positional(0).value().str()), out.positional(1).value().unum() * 8, bind);
		return;
	}
	else if (out.groupId() == L"in-eval") {
		auto msg = out.option(L"desc");
		debugger->printEval((msg.has_value() ? str::u8::To(msg.value().str()) : u8""), str::u8::To(out.positional(0).value().str()), out.positional(1).value().boolean(), bind);
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
		out = arger::Menu(cmd, Commands, 160);
	}
	catch (const arger::PrintMessage& e) {
		util::nullLogger.info(e.what());
		return;
	}
	catch (const arger::ParsingException& e) {
		util::nullLogger.error(e.what(), u8" Use 'help' for more information.");
		return;
	}

	/* handle the create command */
	if (out.groupId() == L"start") {
		if (env::Instance() != 0) {
			util::nullLogger.error(u8"Process is already loaded.");
			return;
		}

		/* system can currently only be 'userspace' and cpu can only be 'rv64' */
		std::wstring cpu = out.option(L"cpu").value().str();
		bool debug = out.flag(L"debug"), logBlocks = out.flag(L"log");
		util::nullLogger.log(u8"Setting up userspace with cpu: [", cpu, u8']');

		/* collect the argument vector */
		std::vector<std::u8string> args;
		for (size_t i = 1; i < out.positionals(); ++i)
			args.push_back(str::u8::To(out.positional(i).value().str()));

		/* collect the environment vector */
		std::vector<std::u8string> envs;
		size_t envCount = out.options(L"environment");
		for (size_t i = 0; i < envCount; ++i)
			envs.push_back(str::u8::To(out.option(L"environment", i).value().str()));

		/* extract the trace type to be used */
		gen::TraceType trace = gen::TraceType::none;
		arger::Value traceValue = out.option(L"trace").value();
		if (traceValue.str() == L"inst")
			trace = gen::TraceType::instruction;
		else if (traceValue.str() == L"chunk")
			trace = gen::TraceType::chunk;
		else if (traceValue.str() == L"block")
			trace = gen::TraceType::block;

		/* try to setup the userspace system */
		debugger = 0;
		if (!sys::Userspace::Create(rv64::Cpu::New(), str::u8::To(out.positional(0).value().str()), args, envs, logBlocks, trace, (debug ? &debugger : 0)))
			util::nullLogger.error(u8"Failed to create process");
		else
			util::nullLogger.log(u8"Process creation completed");
		return;
	}

	/* handle the destroy command */
	if (out.groupId() == L"destroy") {
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
