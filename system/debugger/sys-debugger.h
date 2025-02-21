#pragma once

#include "../sys-common.h"

namespace sys {
	namespace detail {
		struct DebuggerHalt {};
	}

	class Debugger {
		friend class sys::Userspace;
	private:
		enum class Mode : uint8_t {
			disabled,
			none,
			step,
			run,
			until
		};

	private:
		sys::Userspace* pUserspace = 0;
		std::unordered_set<env::guest_t> pBreakPoints;
		std::vector<std::u8string> pRegisters;
		env::guest_t pUntil = 0;
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
		std::vector<uint8_t> fReadBytes(env::guest_t address, size_t bytes) const;
		void fPrintData(env::guest_t address, size_t bytes, uint8_t width) const;
		void fHalted(env::guest_t address);
		void fPrintCommon() const;

	public:
		void run();
		void step(size_t count);
		void until(env::guest_t address);
		void addBreak(env::guest_t address);
		void dropBreak(env::guest_t address);

	public:
		void printState() const;
		void printBreaks() const;
		void printInstructions(std::optional<env::guest_t> address, size_t count) const;
		void printData8(env::guest_t address, size_t bytes) const;
		void printData16(env::guest_t address, size_t bytes) const;
		void printData32(env::guest_t address, size_t bytes) const;
		void printData64(env::guest_t address, size_t bytes) const;
	};
}
