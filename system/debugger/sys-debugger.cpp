#include "../system.h"

static host::Logger logger{ u8"sys::debugger" };
static host::Logger outLog{ u8"" };

namespace global {
	static std::unique_ptr<sys::Debugger> Instance;
}

sys::Debugger* sys::Instance() {
	return global::Instance.get();
}
bool sys::SetInstance(std::unique_ptr<sys::Debuggable>&& debugger) {
	if (global::Instance.get() != 0) {
		logger.error(u8"Cannot create debugger as only one debugger can exist at a time");
		return false;
	}

	/* setup the new instance */
	logger.log(u8"Creating new debugger...");
	global::Instance = std::make_unique<sys::Debugger>();

	/* configure the instance */
	if (detail::DebuggerAccess::Setup(*global::Instance.get(), std::move(debugger))) {
		logger.log(u8"Debugger created");
		return true;
	}
	global::Instance.reset();
	return false;
}
void sys::ClearInstance() {
	logger.log(u8"Destroying debugger...");
	global::Instance.reset();
	logger.log(u8"Debugger destroyed");
}


bool sys::detail::DebuggerAccess::Setup(sys::Debugger& debugger, std::unique_ptr<sys::Debuggable>&& debuggable) {
	return debugger.fSetup(std::move(debuggable));
}


bool sys::Debugger::fSetup(std::unique_ptr<sys::Debuggable>&& debuggable) {
	pDebuggable = std::move(debuggable);

	/* fetch the current cpu and registers */
	pCpu = pDebuggable->getCpu();
	pRegisters = pCpu->queryNames();
	return true;
}
void sys::Debugger::fHalted() {
	pMode = Mode::none;
	printState();
	printInstructions(10);
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
		pDebuggable->run();
}
void sys::Debugger::addBreak(env::guest_t address) {
	pBreakPoints.insert(address);
}
void sys::Debugger::dropBreak(env::guest_t address) {
	pBreakPoints.erase(address);
}
void sys::Debugger::run() {
	pMode = Mode::run;
	pDebuggable->run();
}

void sys::Debugger::printState() const {
	/* print all registers */
	for (size_t i = 0; i < pRegisters.size(); ++i) {
		uintptr_t value = pCpu->getValue(i);
		outLog.log(pRegisters[i], u8": ", str::As{ U"#018x", value }, u8" (", value, u8')');
	}

	/* print the pc */
	env::guest_t addr = pDebuggable->getPC();
	outLog.log(u8"pc: ", str::As{ U"#018x", addr }, u8" (", addr, u8')');
}
void sys::Debugger::printBreaks() const {
	for (env::guest_t addr : pBreakPoints)
		outLog.log(str::As{ U"#018x", addr });
}
void sys::Debugger::printInstructions(size_t count) const {
	env::guest_t addr = pDebuggable->getPC();

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
