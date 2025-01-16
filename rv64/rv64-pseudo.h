#pragma once

#include "rv64-common.h"

namespace rv64 {
	/*
	*	Detect pseudo-instructions and combine them to a single new instruction
	*		Note: will consume instructions until next returns true, in which case it
	*			returns either the combined instruction or the original first instruction
	*		Note: if multi is false, will only ever detect single merged instructions
	*/
	struct DetectPseudo {
	private:
		enum class State : uint8_t {
			none,
			multi,
			auipc,
			lui,
			luiShifted,
			luiAdded
		};
		enum class Result : uint8_t {
			incomplete,
			success,
			restore
		};

	private:
		State pState = State::none;
		struct {
			int64_t imm = 0;
			uint8_t reg = 0;
			uint8_t size = 0;
		} pOriginal;
		struct {
			int64_t imm = 0;
			uint8_t size = 0;
		} pLUI;

	public:
		DetectPseudo(bool multi);

	private:
		void fRestore(rv64::Instruction& inst) const;
		void fCloseLI(rv64::Instruction& inst) const;
		bool fMatchFirst(rv64::Instruction& inst);
		Result fContinueAUIPC(rv64::Instruction& inst) const;
		Result fContinueLUI(rv64::Instruction& inst);

	public:
		bool next(rv64::Instruction& inst);
		rv64::Instruction close() const;
	};
}
