#include "../system.h"

static util::Logger logger{ u8"sys::debugger" };
static util::Logger outLog{ u8"" };

bool sys::Debugger::fActive() const {
	return (pMode != Mode::disabled);
}
bool sys::Debugger::fSetup(sys::Userspace* userspace) {
	pUserspace = userspace;
	pRegisters = pUserspace->cpu()->debugQueryNames();
	return true;
}
bool sys::Debugger::fAdvance(env::guest_t address) {
	if (pMode == Mode::disabled)
		return true;

	/* check if a breakpoint has been hit */
	if (pBreakPoints.contains(address)) {
		fHalted();
		return false;
	}

	/* check if the current mode is considered done */
	switch (pMode) {
	case Mode::step:
		if (--pCount > 0)
			return true;
		break;
	case Mode::run:
		return true;
	default:
		break;
	}

	/* mark the debugger as halted */
	fHalted();
	return false;
}
void sys::Debugger::fHalted() {
	pMode = Mode::none;
}

void sys::Debugger::step(size_t count) {
	pMode = Mode::step;
	if ((pCount = count) > 0)
		pUserspace->execute();
}
void sys::Debugger::addBreak(env::guest_t address) {
	pBreakPoints.insert(address);
}
void sys::Debugger::dropBreak(env::guest_t address) {
	pBreakPoints.erase(address);
}
void sys::Debugger::run() {
	pMode = Mode::run;
	pUserspace->execute();
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
