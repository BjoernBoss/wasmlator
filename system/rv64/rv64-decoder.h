#pragma once

#include "rv64-common.h"

namespace rv64 {
	namespace detail {
		static constexpr uint8_t RVCRegisters[8] = { 8, 9, 10, 11, 12, 13, 14, 15 };

		template <size_t First, size_t Last>
		uint32_t GetU(uint32_t data) {
			static constexpr size_t Count = (Last - First + 1);
			return (data >> First) & ((1 << Count) - 1);
		}

		template <size_t First, size_t Last>
		int32_t GetS(uint32_t data) {
			static constexpr size_t Count = (Last - First + 1);

			uint32_t v = detail::GetU<First, Last>(data);
			if ((v >> (Count - 1)) & 0x01)
				v |= (uint64_t(0xffffffff) << Count);
			return int32_t(v);
		}

		rv64::Instruction Opcode03(uint32_t data);
		rv64::Instruction Opcode13(uint32_t data);
		rv64::Instruction Opcode17(uint32_t data);
		rv64::Instruction Opcode23(uint32_t data);
		rv64::Instruction Opcode33(uint32_t data);
		rv64::Instruction Opcode37(uint32_t data);
		rv64::Instruction Opcode63(uint32_t data);
		rv64::Instruction Opcode67(uint32_t data);
		rv64::Instruction Opcode6f(uint32_t data);

		rv64::Instruction Quadrant0(uint16_t data);
		rv64::Instruction Quadrant1(uint16_t data);
		rv64::Instruction Quadrant2(uint16_t data);
	}

	/*
	*	Decodes: rv32i, rv32m, rv64i
	*/
	rv64::Instruction Decode32(uint32_t data);

	/*
	*	Decodes: rvc with rv64 configuration
	*/
	rv64::Instruction Decode16(uint16_t data);
}
