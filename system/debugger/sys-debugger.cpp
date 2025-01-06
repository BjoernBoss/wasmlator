#include "../system.h"

static host::Logger logger{ u8"sys::debugger" };

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
	return true;
}
bool sys::Debugger::completed(env::guest_t address) {
	/* check if the current mode is considered done */
	switch (pMode) {
	case Mode::step:
		if (--pCount > 0)
			return true;
		break;
	default:
		break;
	}

	/* reset the mode and mark the stepping as done */
	pMode = Mode::none;
	return false;
}
void sys::Debugger::step(size_t count) {
	pMode = Mode::step;
	if ((pCount = count) > 0)
		pDebuggable->run();
}
