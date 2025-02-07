#include "../system.h"

static util::Logger logger{ u8"sys::debugger" };
static util::Logger outLog{ u8"" };

bool sys::Debugger::fSetup(sys::Userspace* userspace) {
	pUserspace = userspace;
	pRegisters = pUserspace->cpu()->debugQueryNames();
	pMode = Mode::none;
	return true;
}
void sys::Debugger::fCheck(env::guest_t address) {
	if (pMode == Mode::disabled)
		return;

	/* check if a breakpoint has been hit */
	if (pBreakPoints.contains(address) && !pBreakSkip) {
		fHalted(address);
		return;
	}
	pBreakSkip = false;

	/* check if the current mode is considered done */
	switch (pMode) {
	case Mode::step:
		if (pCount-- > 0)
			return;
		break;
	case Mode::run:
		return;
	default:
		break;
	}

	/* mark the debugger as halted */
	fHalted(address);
}

void sys::Debugger::fHalted(env::guest_t address) {
	/* reset the debugger */
	pMode = Mode::none;
	pCount = 0;

	/* print common halting information (set the pc beforehand, as it is used by the printing) */
	pUserspace->setPC(address);
	fPrintCommon();

	throw detail::DebuggerHalt{ address };
}
void sys::Debugger::fPrintCommon() const {
	printState();
	printInstructions(10);
}

void sys::Debugger::run() {
	pMode = Mode::run;
	pBreakSkip = true;
	pUserspace->execute();
}
void sys::Debugger::step(size_t count) {
	pMode = Mode::step;
	pBreakSkip = true;
	if ((pCount = count) > 0)
		pUserspace->execute();
}
void sys::Debugger::addBreak(env::guest_t address) {
	pBreakPoints.insert(address);
}
void sys::Debugger::dropBreak(env::guest_t address) {
	pBreakPoints.erase(address);
}

void sys::Debugger::printState() const {
	sys::Cpu* cpu = pUserspace->cpu();

	/* print all registers */
	for (size_t i = 0; i < pRegisters.size(); ++i) {
		uint64_t value = cpu->debugGetValue(i);
		outLog.log(pRegisters[i], u8": ", str::As{ U"#018x", value }, u8" (", value, u8')');
	}

	/* print the pc */
	env::guest_t addr = pUserspace->getPC();
	outLog.log(u8"pc: ", str::As{ U"#018x", addr }, u8" (", addr, u8')');
}
void sys::Debugger::printBreaks() const {
	for (env::guest_t addr : pBreakPoints)
		outLog.log(str::As{ U"#018x", addr });
}
void sys::Debugger::printInstructions(size_t count) const {
	env::guest_t addr = pUserspace->getPC();
	sys::Cpu* cpu = pUserspace->cpu();

	try {
		while (count-- > 0) {
			/* decode the next instruction and check if the decoding failed */
			auto [str, size] = cpu->debugDecode(addr);
			if (size == 0) {
				outLog.fmtLog(u8"[{:#018x}]: Unable to decode", addr);
				break;
			}

			/* print the instruction and advance the address */
			outLog.fmtLog(u8"[{:#018x}]: {}", addr, str);
			addr += size;
		}
	}

	/* check if a memory fault occurred */
	catch (const env::MemoryFault&) {
		outLog.fmtLog(u8"[{:#018x}]: Unable to read memory", addr);
	}
}
