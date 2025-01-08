#include "sys-debugger.h"

static host::Logger logger{ u8"sys::debugger" };
static host::Logger outLog{ u8"" };

void sys::Debugger::fHalted() {
	pMode = Mode::none;
}

std::unique_ptr<sys::Debugger> sys::Debugger::New(std::unique_ptr<sys::Debuggable> provider) {
	std::unique_ptr<sys::Debugger> debugger{ new sys::Debugger() };

	/* setup the debugger host */
	debugger->pProvider = std::move(provider);
	debugger->pCpu = debugger->pProvider->getCpu();
	if (debugger->pCpu.get() == 0)
		return 0;
	debugger->pRegisters = debugger->pCpu->queryNames();
	return debugger;
}

bool sys::Debugger::advance(env::guest_t address) {
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
void sys::Debugger::step(size_t count) {
	pMode = Mode::step;
	if ((pCount = count) > 0)
		pProvider->run();
}
void sys::Debugger::addBreak(env::guest_t address) {
	pBreakPoints.insert(address);
}
void sys::Debugger::dropBreak(env::guest_t address) {
	pBreakPoints.erase(address);
}
void sys::Debugger::run() {
	pMode = Mode::run;
	pProvider->run();
}

void sys::Debugger::printState() const {
	/* print all registers */
	for (size_t i = 0; i < pRegisters.size(); ++i) {
		uint64_t value = pCpu->getValue(i);
		outLog.log(pRegisters[i], u8": ", str::As{ U"#018x", value }, u8" (", value, u8')');
	}

	/* print the pc */
	env::guest_t addr = pProvider->getPC();
	outLog.log(u8"pc: ", str::As{ U"#018x", addr }, u8" (", addr, u8')');
}
void sys::Debugger::printBreaks() const {
	for (env::guest_t addr : pBreakPoints)
		outLog.log(str::As{ U"#018x", addr });
}
void sys::Debugger::printInstructions(size_t count) const {
	env::guest_t addr = pProvider->getPC();

	try {
		while (count-- > 0) {
			/* decode the next instruction and check if the decoding failed */
			auto [str, size] = pCpu->decode(addr);
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
