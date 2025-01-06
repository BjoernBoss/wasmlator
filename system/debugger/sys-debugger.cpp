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
		logger.error(u8"Cannot attach debugger as only one debugger can exist at a time");
		return false;
	}

	/* configure the instance */
	if (detail::DebuggerAccess::Setup(*global::Instance.get(), std::move(debugger))) {
		logger.log(u8"Debugger instance attached");
		return true;
	}
	global::Instance.reset();
	return false;
}
void sys::ClearInstance() {
	global::Instance.reset();
	logger.log(u8"Debugger detached");
}


bool sys::detail::DebuggerAccess::Setup(sys::Debugger& debugger, std::unique_ptr<sys::Debuggable>&& debuggable) {
	return debugger.fSetup(std::move(debuggable));
}


bool sys::Debugger::fSetup(std::unique_ptr<sys::Debuggable>&& debuggable) {
	pDebuggable = std::move(debuggable);
	return true;
}
bool sys::Debugger::completed() const {
	return true;
}
void sys::Debugger::step(size_t count) {

}
