#pragma once

#include "rv64-common.h"

namespace rv64 {
	/* performs primitive macro-expansion-like translation currently for single-threaded userspace processes */
	class Translate {
	private:
		wasm::Variable pDivisionTemp32;
		wasm::Variable pDivisionTemp64;
		sys::ExecContext* pContext = 0;
		const gen::Writer* pWriter = 0;
		const rv64::Instruction* pInst = 0;
		env::guest_t pAddress = 0;
		env::guest_t pNextAddress = 0;

	public:
		Translate() = default;

	private:
		bool fLoadSrc1(bool forceNull) const;
		bool fLoadSrc1Half(bool forceNull) const;
		bool fLoadSrc2(bool forceNull) const;
		bool fLoadSrc2Half(bool forceNull) const;
		void fStoreDest() const;

	private:
		void fMakeJAL() const;
		void fMakeBranch() const;
		void fMakeALUImm() const;
		void fMakeALUReg() const;
		void fMakeLoad() const;
		void fMakeStore() const;
		void fMakeMul() const;
		void fMakeDivRem();
		void fMakeAMO() const;

	public:
		void resetAll(sys::ExecContext* context, const gen::Writer* writer);
		void start(env::guest_t address);
		void next(const rv64::Instruction& inst);
	};
}
