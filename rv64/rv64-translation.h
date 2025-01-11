#pragma once

#include "rv64-common.h"

namespace rv64 {
	/* performs primitive macro-expansion-like translation currently for single-threaded userspace processes */
	class Translate {
	public:
		static constexpr uint64_t EBreakException = 0;
		static constexpr uint64_t MisAlignedException = 1;

	private:
		wasm::Variable pTemp[6];
		sys::Writer* pWriter = 0;
		const rv64::Instruction* pInst = 0;
		env::guest_t pAddress = 0;
		env::guest_t pNextAddress = 0;

	public:
		Translate() = default;

	private:
		wasm::Variable fTemp32(size_t index);
		wasm::Variable fTemp64(size_t index);

	private:
		bool fLoadSrc1(bool forceNull, bool half) const;
		bool fLoadSrc2(bool forceNull, bool half) const;
		void fStoreDest() const;

	private:
		void fMakeJAL();
		void fMakeBranch() const;
		void fMakeALUImm() const;
		void fMakeALUReg() const;
		void fMakeLoad() const;
		void fMakeStore() const;
		void fMakeDivRem();
		void fMakeAMO(bool half);
		void fMakeAMOLR();
		void fMakeAMOSC();
		void fMakeCSR() const;
		void fMakeMul();

	public:
		void resetAll(sys::Writer* writer);
		void start(env::guest_t address);
		void next(const rv64::Instruction& inst);
	};
}
