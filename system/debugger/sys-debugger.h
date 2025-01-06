#pragma once

#include "../sys-common.h"

namespace sys {
	namespace detail {
		struct DebuggerAccess {
			static bool Setup(sys::Debugger& debugger, std::unique_ptr<sys::Debuggable>&& debuggable);
		};
	}

	class Debugger {
		friend struct detail::DebuggerAccess;
	private:
		std::unique_ptr<sys::Debuggable> pDebuggable;

	protected:
		Debugger() = default;

	public:
		virtual ~Debugger() = default;

	private:
		bool fSetup(std::unique_ptr<sys::Debuggable>&& debuggable);

	public:
		bool completed() const;

	public:
		void step(size_t count);
	};
}
