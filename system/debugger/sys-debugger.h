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
		enum class Mode : uint8_t {
			none,
			step
		};

	private:
		std::unique_ptr<sys::Debuggable> pDebuggable;
		size_t pCount = 0;
		Mode pMode = Mode::none;

	public:
		Debugger() = default;
		Debugger(sys::Debugger&&) = delete;
		Debugger(const sys::Debugger&) = delete;
		~Debugger() = default;

	private:
		bool fSetup(std::unique_ptr<sys::Debuggable>&& debuggable);

	public:
		/* to be executed after an instruction has been executed and returns true, if another step should be taken */
		bool completed(env::guest_t address);

	public:
		void step(size_t count);
	};
}
