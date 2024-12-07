#pragma once

#include "../generate/generate.h"

#include "rv64-common.h"

namespace rv64 {
	/* performs primitive macro-expansion-like translation */
	class Translate {
	private:
		const gen::Writer& pWriter;
		env::guest_t pAddress = 0;
		env::guest_t pNextAddress = 0;
		size_t pSize = 0;
		const rv64::Instruction* pInst = 0;

	public:
		Translate(const gen::Writer& writer, env::guest_t address);

	private:
		bool fLoadSrc1(bool forceNull) const;
		bool fLoadSrc2(bool forceNull) const;
		void fStoreDest() const;

	private:
		void fMakeJAL() const;
		void fMakeBranch() const;
		void fMakeALUImm() const;
		void fMakeALUReg() const;
		void fMakeLoad() const;
		void fMakeStore() const;

	public:
		void next(const rv64::Instruction& inst);
	};
}
