#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <memory>
#include <vector>

#include "../interface/host.h"

/* riscv 64-bit */
namespace rv64 {
	/* throw exception if jump-target is not 4-byte aligned */
	struct Context {
		union {
			uint64_t x00 = 0;
			uint64_t _zero;
		};
		union {
			uint64_t x01 = 0;
			uint64_t ra;
		};
		union {
			uint64_t x02 = 0;
			uint64_t sp;
		};
		union {
			uint64_t x03 = 0;
			uint64_t gp;
		};
		union {
			uint64_t x04 = 0;
			uint64_t tp;
		};
		union {
			uint64_t x05 = 0;
			uint64_t t0;
		};
		union {
			uint64_t x06 = 0;
			uint64_t t1;
		};
		union {
			uint64_t x07 = 0;
			uint64_t t2;
		};
		union {
			uint64_t x08 = 0;
			uint64_t fp;
			uint64_t s0;
		};
		union {
			uint64_t x09 = 0;
			uint64_t s1;
		};
		union {
			uint64_t x10 = 0;
			uint64_t a0;
		};
		union {
			uint64_t x11 = 0;
			uint64_t a1;
		};
		union {
			uint64_t x12 = 0;
			uint64_t a2;
		};
		union {
			uint64_t x13 = 0;
			uint64_t a3;
		};
		union {
			uint64_t x14 = 0;
			uint64_t a4;
		};
		union {
			uint64_t x15 = 0;
			uint64_t a5;
		};
		union {
			uint64_t x16 = 0;
			uint64_t a6;
		};
		union {
			uint64_t x17 = 0;
			uint64_t a7;
		};
		union {
			uint64_t x18 = 0;
			uint64_t s2;
		};
		union {
			uint64_t x19 = 0;
			uint64_t s3;
		};
		union {
			uint64_t x20 = 0;
			uint64_t s4;
		};
		union {
			uint64_t x21 = 0;
			uint64_t s5;
		};
		union {
			uint64_t x22 = 0;
			uint64_t s6;
		};
		union {
			uint64_t x23 = 0;
			uint64_t s7;
		};
		union {
			uint64_t x24 = 0;
			uint64_t s8;
		};
		union {
			uint64_t x25 = 0;
			uint64_t s9;
		};
		union {
			uint64_t x26 = 0;
			uint64_t s10;
		};
		union {
			uint64_t x27 = 0;
			uint64_t s11;
		};
		union {
			uint64_t x28 = 0;
			uint64_t t3;
		};
		union {
			uint64_t x29 = 0;
			uint64_t t4;
		};
		union {
			uint64_t x30 = 0;
			uint64_t t5;
		};
		union {
			uint64_t x31 = 0;
			uint64_t t6;
		};
	};

	enum class Opcode : uint16_t {
		load_upper_imm,
		add_upper_imm_pc,
		jump_and_link_imm,
		jump_and_link_reg,
		branch_eq,
		branch_ne,
		branch_lt_s,
		branch_ge_s,
		branch_lt_u,
		branch_ge_u,
		add_imm,
		set_less_than_s_imm,
		set_less_than_u_imm,
		xor_imm,
		or_imm,
		and_imm,
		shift_left_logic_imm,
		shift_right_logic_imm,
		shift_right_arith_imm,
		add_reg,
		sub_reg,
		mul_reg,
		mul_high_s_reg,
		mul_high_s_u_reg,
		mul_high_u_reg,
		div_s_reg,
		div_u_reg,
		rem_s_reg,
		rem_u_reg,
		set_less_than_s_reg,
		set_less_than_u_reg,
		xor_reg,
		or_reg,
		and_reg,
		shift_left_logic_reg,
		shift_right_logic_reg,
		shift_right_arith_reg,
		no_op,
		move,
		_invalid
	};
	enum class Format : uint8_t {
		none,
		dst_src1,
		dst_src1_imm,
		dst_src1_src2,
		dst_imm,
		src1_src2
	};

	namespace reg {
		static constexpr uint8_t Zero = 0;
		static constexpr uint8_t X1 = 1;
		static constexpr uint8_t X5 = 5;
	}

	struct Instruction {
	public:
		rv64::Opcode opcode = rv64::Opcode::_invalid;
		rv64::Format format = Format::none;
		uint8_t dest = 0;
		uint8_t src1 = 0;
		uint8_t src2 = 0;
		int64_t imm = 0;

	public:
		constexpr bool isCall() const {
			/* for jal-s with immediate, check for write to x1/x5 */
			if (opcode == rv64::Opcode::jump_and_link_imm)
				return (dest == reg::X1 || dest == reg::X5);

			/* for jal-s with register, check for write to x1/x5 and
			*	either no read from x1/x5 or identical read/write */
			else if (opcode == rv64::Opcode::jump_and_link_reg) {
				if (dest != reg::X1 && dest != reg::X5)
					return false;
				if (src1 != reg::X1 && src1 != reg::X5)
					return true;
				return (src1 == dest);
			}
			return false;
		}
		constexpr bool isRet() const {
			/* for jal-s with register, check for no write to x1/x5 and read from x1/x5 */
			if (opcode != rv64::Opcode::jump_and_link_reg)
				return false;
			if (dest == reg::X1 || dest == reg::X5)
				return false;
			return (src1 == reg::X1 || src1 == reg::X5);
		}
	};
}
