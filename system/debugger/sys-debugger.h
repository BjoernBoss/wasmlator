#pragma once

#include "../sys-common.h"

namespace sys {
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

	private:
		Debugger() = default;
		Debugger(sys::Debugger&&) = delete;
		Debugger(const sys::Debugger&) = delete;

	private:
		bool fActive() const;
		bool fSetup(sys::Userspace* userspace);
		bool fAdvance(env::guest_t address);
		void fHalted();

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
