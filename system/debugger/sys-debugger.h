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
			step,
			run
		};

	private:
		std::unique_ptr<sys::Debuggable> pDebuggable;
		std::unordered_set<env::guest_t> pBreakPoints;
		std::vector<std::u8string> pRegisters;
		sys::Cpu* pCpu = 0;
		size_t pCount = 0;
		Mode pMode = Mode::none;

	public:
		Debugger() = default;
		Debugger(sys::Debugger&&) = delete;
		Debugger(const sys::Debugger&) = delete;
		~Debugger() = default;

	private:
		bool fSetup(std::unique_ptr<sys::Debuggable>&& debuggable);
		void fHalted();

	public:
		/* to be executed after an instruction has been executed and returns true, if another step should be taken */
		bool completed(env::guest_t address);

	public:
		void step(size_t count);
		void addBreak(env::guest_t address);
		void dropBreak(env::guest_t address);
		void run();

	public:
		void printState() const;
		void printBreaks() const;
		void printInstructions(size_t count) const;
	};
}
