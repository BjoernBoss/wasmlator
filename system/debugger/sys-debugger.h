#pragma once

#include "../sys-common.h"

namespace sys {
	namespace detail {
		struct DebuggerHalt : public env::Exception {
		public:
			DebuggerHalt(env::guest_t address) : env::Exception{ address } {}
		};
	}

	class Debugger {
		friend class sys::Userspace;
	private:
		enum class Mode : uint8_t {
			disabled,
			none,
			step,
			run
		};

	private:
		sys::Userspace* pUserspace = 0;
		std::unordered_set<env::guest_t> pBreakPoints;
		std::vector<std::u8string> pRegisters;
		size_t pCount = 0;
		Mode pMode = Mode::disabled;
		bool pBreakSkip = false;

	private:
		Debugger() = default;
		Debugger(sys::Debugger&&) = delete;
		Debugger(const sys::Debugger&) = delete;

	private:
		bool fSetup(sys::Userspace* userspace);
		void fCheck(env::guest_t address);

	private:
		void fHalted(env::guest_t address);
		void fPrintCommon() const;

	public:
		void run();
		void step(size_t count);
		void addBreak(env::guest_t address);
		void dropBreak(env::guest_t address);

	public:
		void printState() const;
		void printBreaks() const;
		void printInstructions(size_t count) const;
	};
}
