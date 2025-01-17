#pragma once

#include "rv64-common.h"

namespace rv64 {
	/* performs primitive macro-expansion-like translation currently for single-threaded userspace processes */
	class Translate {
	public:
		static constexpr uint64_t EBreakException = 0;
		static constexpr uint64_t MisalignedException = 1;
		static constexpr uint64_t IllegalException = 2;
		static constexpr uint64_t NotImplException = 3;

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
		void fStoreReg(uint8_t reg) const;
		void fStoreDest() const;

	private:
		void fLoadFSrc1(bool half) const;
		void fLoadFSrc2(bool half) const;
		void fLoadFSrc3(bool half) const;
		void fStoreFDest(bool isAsInt) const;
		void fExpandFloat(bool isAsInt, bool leaveAsInt) const;

	private:
		void fMakeImms() const;
		void fMakeJALI();
		void fMakeJALR();
		void fMakeBranch() const;
		void fMakeALUImm() const;
		void fMakeALUReg() const;
		void fMakeLoad(bool multi) const;
		void fMakeStore(bool multi) const;
		void fMakeDivRem();
		void fMakeAMO(bool half);
		void fMakeAMOLR();
		void fMakeAMOSC();
		void fMakeMul();
		void fMakeCSR();

	private:
		void fMakeFLoad(bool multi) const;
		void fMakeFStore(bool multi) const;

	public:
		void resetAll(sys::Writer* writer);
		void start(env::guest_t address);
		void next(const rv64::Instruction& inst);
	};
}
