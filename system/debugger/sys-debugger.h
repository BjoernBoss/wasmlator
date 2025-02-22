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
		enum class BindType : uint8_t {
			echo,
			evalDec,
			evalHex,
			state,
			inst,
			data8,
			data16,
			data32,
			data64,
			_end
		};
		struct Operation {
			size_t regIndex = 0;
			uint64_t immediate = 0;
			bool useRegister = false;
			bool usePC = false;
			bool subtract = false;
		};
		using Expression = std::vector<Operation>;
		struct PrintBinding {
			Expression expression;
			std::u8string msg;
			size_t index = 0;
			size_t misc = 0;
			BindType type = BindType::_end;
		};

	private:
		sys::Userspace* pUserspace = 0;
		sys::Cpu* pCpu = 0;
		std::unordered_set<env::guest_t> pBreakPoints;
		std::map<size_t, env::guest_t> pBreakIndices;
		std::vector<std::u8string> pRegisters;
		std::vector<PrintBinding> pBindings;
		size_t pNextBreakPoint = 0;
		size_t pNextBound = 0;
		struct {
			env::guest_t until = 0;
			size_t count = 0;
			Mode mode = Mode::disabled;
			bool breakSkip = false;
		} pHalt;

	private:
		Debugger() = default;
		Debugger(sys::Debugger&&) = delete;
		Debugger(const sys::Debugger&) = delete;

	private:
		bool fSetup(sys::Userspace* userspace);
		void fCheck(env::guest_t address);
		void fHalted(env::guest_t address);

	private:
		void fAddBinding(const PrintBinding& binding);
		std::vector<uint8_t> fReadBytes(env::guest_t address, size_t bytes) const;
		bool fParseExpression(std::u8string_view exp, Expression& out) const;
		uint64_t fEvalExpression(const Expression& ops) const;
		std::optional<uint64_t> fParseAndEval(std::u8string_view exp) const;

	private:
		void fPrintInstructions(const Expression& address, size_t count) const;
		void fPrintData(const Expression& address, size_t bytes, uint8_t width) const;
		void fPrintEval(const Expression& value, const std::u8string& msg, bool hex) const;
		void fPrintState() const;
		void fPrintBindings() const;

	public:
		void run();
		void step(size_t count);
		void until(const std::u8string& address);
		void addBreak(const std::u8string& address);
		void dropBreak(size_t index);
		void dropBinding(size_t index);
		void setupCommon(std::optional<std::u8string> spName);

	public:
		void printBreaks() const;
		void printBindings() const;
		void printEcho(const std::u8string& msg, bool bind);
		void printEval(const std::u8string& msg, const std::u8string& value, bool hex, bool bind);
		void printState(bool bind);
		void printInstructions(std::optional<std::u8string> address, size_t count, bool bind);
		void printData8(const std::u8string& address, size_t bytes, bool bind);
		void printData16(const std::u8string& address, size_t bytes, bool bind);
		void printData32(const std::u8string& address, size_t bytes, bool bind);
		void printData64(const std::u8string& address, size_t bytes, bool bind);
	};
}
