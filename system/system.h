#pragma once

#include "sys-common.h"
#include "rv64/rv64-cpu.h"
#include "debugger/sys-debugger.h"
#include "primitive/sys-primitive.h"

namespace sys {
	/* Many operations on the debugger require both the system instance and core to be loaded properly.
	*	Note: shutdown must be performed through env::System::shutdown, and must only be exeucted from within external calls */
	sys::Debugger* Instance();
	bool SetInstance(std::unique_ptr<sys::Debuggable>&& debuggable);
	void ClearInstance();
}
