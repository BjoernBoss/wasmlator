/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "rv64-common.h"

namespace rv64 {
	/* performs primitive macro-expansion-like translation currently for single-threaded userspace processes */
	class Translate {
	public:
		static constexpr uint64_t EBreakException = 0;
		static constexpr uint64_t MisalignedException = 1;
		static constexpr uint64_t IllegalException = 2;
		static constexpr uint64_t CsrUnsupported = 3;
		static constexpr uint64_t NotImplException = 4;

	private:
		wasm::Variable pTemp[8];
		sys::Writer* pWriter = 0;
		const rv64::Instruction* pInst = 0;
		env::guest_t pAddress = 0;
		env::guest_t pNextAddress = 0;
		struct {
			uint32_t classify32Bit = 0;
			uint32_t classify64Bit = 0;
			uint32_t readFloatCsrWarn = 0;
			uint32_t frmFloatCsrWarn = 0;
		} pRegistered;
		bool pReadCsrShown = false;
		bool pFrmFloatShown = false;

	public:
		Translate() = default;

	private:
		std::pair<uint32_t, uint32_t> fGetCsrPlacement(uint16_t csr) const;
		wasm::Variable fTempi32(size_t index);
		wasm::Variable fTempi64(size_t index);
		wasm::Variable fTempf32();
		wasm::Variable fTempf64();

	private:
		static uint32_t fClassifyFloat(int _class, bool _sign, bool _topMantissa);
		static uint32_t fClassifyf32(float value);
		static uint32_t fClassifyf64(double value);

	private:
		bool fLoadSrc1(bool forceNull, bool half) const;
		bool fLoadSrc2(bool forceNull, bool half) const;
		gen::FulFill fStoreReg(uint8_t reg) const;
		gen::FulFill fStoreDest() const;

	private:
		void fLoadFSrc1(bool half) const;
		void fLoadFSrc2(bool half) const;
		void fLoadFSrc3(bool half) const;
		gen::FulFill fStoreFDest(bool isAsInt) const;
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
		void fMakeFloatToInt(bool iHalf, bool fHalf);
		void fMakeIntToFloat(bool iHalf, bool fHalf) const;
		void fMakeFloatALUSimple(bool half) const;
		void fMakeFloatALULarge(bool half) const;
		void fMakeFloatConvert() const;
		void fMakeFloatSign(bool half) const;
		void fMakeFloatCompare(bool half) const;
		void fMakeFloatUnary(bool half, bool intResult) const;

	public:
		bool setup();
		void resetAll(sys::Writer* writer);
		void start(env::guest_t address);
		void next(const rv64::Instruction& inst);
	};
}
