#pragma once

#include "sys-debuggable.h"

namespace sys {
	/* Note: returns nullptr on construction failure */
	class Debugger {
	private:
		enum class Mode : uint8_t {
			none,
			step,
			run
		};

	private:
		std::unique_ptr<sys::Debuggable> pProvider;
		std::unordered_set<env::guest_t> pBreakPoints;
		std::vector<std::u8string> pRegisters;
		size_t pCount = 0;
		Mode pMode = Mode::none;

	private:
		Debugger() = default;
		Debugger(sys::Debugger&&) = delete;
		Debugger(const sys::Debugger&) = delete;

	private:
		void fHalted();

	public:
		static std::unique_ptr<sys::Debugger> New(std::unique_ptr<sys::Debuggable> provider);

	public:
		/* to be executed after an instruction has been executed and returns true, if another step should be taken */
		bool advance(env::guest_t address);

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
