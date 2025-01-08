#pragma once

#include "../sys-common.h"

namespace sys {
	class CpuDebuggable {
	protected:
		CpuDebuggable() = default;

	public:
		virtual ~CpuDebuggable() = default;

	public:
		/* fetch the name of all supported registers */
		virtual std::vector<std::u8string> queryNames() const = 0;

		/* decode the instruction at the address and return its size (size of null implicates decoding failure, may throw env::MemoryFault) */
		virtual std::pair<std::u8string, uint8_t> decode(uintptr_t address) const = 0;

		/* read the current cpu value (index matches the queried name-index) */
		virtual uintptr_t getValue(size_t index) const = 0;

		/* set a value of the current cpu state (index matches the queried name-index) */
		virtual void setValue(size_t index, uintptr_t value) = 0;
	};

	class Debuggable {
	protected:
		Debuggable() = default;

	public:
		virtual ~Debuggable() = default;

	public:
		/* get the debug-interface to the cpu (return nullptr if not supported) */
		virtual std::unique_ptr<sys::CpuDebuggable> getCpu() = 0;

		/* get the current pc */
		virtual env::guest_t getPC() const = 0;

		/* set the current pc */
		virtual void setPC(env::guest_t pc) = 0;

		/* continue execution until the debugger returns true for completed */
		virtual void run() = 0;
	};
}
