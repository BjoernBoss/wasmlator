#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <memory>
#include <vector>

#include "../system/system.h"

/* riscv 64-bit */
namespace rv64 {
	/* one cache per register (no differentiation between reading and writing - done by generator) */
	static constexpr uint32_t MemoryCaches = 32;

	struct Context {
		union {
			uint64_t iregs[32] = { 0 };
			struct {
				union {
					uint64_t x00;
					uint64_t _zero;
				};
				union {
					uint64_t x01;
					uint64_t ra;
				};
				union {
					uint64_t x02;
					uint64_t sp;
				};
				union {
					uint64_t x03;
					uint64_t gp;
				};
				union {
					uint64_t x04;
					uint64_t tp;
				};
				union {
					uint64_t x05;
					uint64_t t0;
				};
				union {
					uint64_t x06;
					uint64_t t1;
				};
				union {
					uint64_t x07;
					uint64_t t2;
				};
				union {
					uint64_t x08;
					uint64_t fp;
					uint64_t s0;
				};
				union {
					uint64_t x09;
					uint64_t s1;
				};
				union {
					uint64_t x10;
					uint64_t a0;
				};
				union {
					uint64_t x11;
					uint64_t a1;
				};
				union {
					uint64_t x12;
					uint64_t a2;
				};
				union {
					uint64_t x13;
					uint64_t a3;
				};
				union {
					uint64_t x14;
					uint64_t a4;
				};
				union {
					uint64_t x15;
					uint64_t a5;
				};
				union {
					uint64_t x16;
					uint64_t a6;
				};
				union {
					uint64_t x17;
					uint64_t a7;
				};
				union {
					uint64_t x18;
					uint64_t s2;
				};
				union {
					uint64_t x19;
					uint64_t s3;
				};
				union {
					uint64_t x20;
					uint64_t s4;
				};
				union {
					uint64_t x21;
					uint64_t s5;
				};
				union {
					uint64_t x22;
					uint64_t s6;
				};
				union {
					uint64_t x23;
					uint64_t s7;
				};
				union {
					uint64_t x24;
					uint64_t s8;
				};
				union {
					uint64_t x25;
					uint64_t s9;
				};
				union {
					uint64_t x26;
					uint64_t s10;
				};
				union {
					uint64_t x27;
					uint64_t s11;
				};
				union {
					uint64_t x28;
					uint64_t t3;
				};
				union {
					uint64_t x29;
					uint64_t t4;
				};
				union {
					uint64_t x30;
					uint64_t t5;
				};
				union {
					uint64_t x31;
					uint64_t t6;
				};
			};
		};
		double fregs[32] = { 0 };
		uint64_t float_csr = 0;
	};

	enum class Opcode : uint16_t {
		misaligned,
		illegal_instruction,

		multi_load_imm,
		multi_load_address,
		multi_load_byte_s,
		multi_load_half_s,
		multi_load_word_s,
		multi_load_byte_u,
		multi_load_half_u,
		multi_load_word_u,
		multi_load_dword,
		multi_load_float,
		multi_load_double,
		multi_store_byte,
		multi_store_half,
		multi_store_word,
		multi_store_dword,
		multi_store_float,
		multi_store_double,
		multi_call,
		multi_tail,

		nop,
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

		load_byte_s,
		load_half_s,
		load_word_s,
		load_byte_u,
		load_half_u,
		load_word_u,
		load_dword,
		store_byte,
		store_half,
		store_word,
		store_dword,

		add_imm,
		add_imm_half,
		xor_imm,
		or_imm,
		and_imm,
		shift_left_logic_imm,
		shift_left_logic_imm_half,
		shift_right_logic_imm,
		shift_right_logic_imm_half,
		shift_right_arith_imm,
		shift_right_arith_imm_half,
		set_less_than_s_imm,
		set_less_than_u_imm,

		add_reg,
		add_reg_half,
		sub_reg,
		sub_reg_half,
		xor_reg,
		or_reg,
		and_reg,
		shift_left_logic_reg,
		shift_left_logic_reg_half,
		shift_right_logic_reg,
		shift_right_logic_reg_half,
		shift_right_arith_reg,
		shift_right_arith_reg_half,
		set_less_than_s_reg,
		set_less_than_u_reg,

		fence,
		fence_inst,
		ebreak,
		ecall,
		csr_read_write,
		csr_read_and_set,
		csr_read_and_clear,
		csr_read_write_imm,
		csr_read_and_set_imm,
		csr_read_and_clear_imm,

		mul_reg,
		mul_reg_half,
		mul_high_s_reg,
		mul_high_s_u_reg,
		mul_high_u_reg,
		div_s_reg,
		div_s_reg_half,
		div_u_reg,
		div_u_reg_half,
		rem_s_reg,
		rem_s_reg_half,
		rem_u_reg,
		rem_u_reg_half,

		load_reserved_w,
		store_conditional_w,
		amo_swap_w,
		amo_add_w,
		amo_xor_w,
		amo_and_w,
		amo_or_w,
		amo_min_s_w,
		amo_max_s_w,
		amo_min_u_w,
		amo_max_u_w,
		load_reserved_d,
		store_conditional_d,
		amo_swap_d,
		amo_add_d,
		amo_xor_d,
		amo_and_d,
		amo_or_d,
		amo_min_s_d,
		amo_max_s_d,
		amo_min_u_d,
		amo_max_u_d,

		load_float,
		load_double,
		store_float,
		store_double,

		float_mul_add,
		float_mul_sub,
		float_neg_mul_add,
		float_neg_mul_sub,
		float_add,
		float_sub,
		float_mul,
		float_div,
		float_sqrt,
		float_sign_copy,
		float_sign_invert,
		float_sign_xor,
		float_min,
		float_max,
		float_less_equal,
		float_less_than,
		float_equal,
		float_classify,

		double_mul_add,
		double_mul_sub,
		double_neg_mul_add,
		double_neg_mul_sub,
		double_add,
		double_sub,
		double_mul,
		double_div,
		double_sqrt,
		double_sign_copy,
		double_sign_invert,
		double_sign_xor,
		double_min,
		double_max,
		double_less_equal,
		double_less_than,
		double_equal,
		double_classify,

		float_convert_to_word_s,
		float_convert_to_word_u,
		float_convert_to_dword_s,
		float_convert_to_dword_u,
		double_convert_to_word_s,
		double_convert_to_word_u,
		double_convert_to_dword_s,
		double_convert_to_dword_u,
		float_convert_from_word_s,
		float_convert_from_word_u,
		float_convert_from_dword_s,
		float_convert_from_dword_u,
		double_convert_from_word_s,
		double_convert_from_word_u,
		double_convert_from_dword_s,
		double_convert_from_dword_u,
		float_move_to_word,
		float_move_from_word,
		double_move_to_dword,
		double_move_from_dword,
		float_to_double,
		double_to_float,

		_invalid
	};
	enum class Pseudo : uint8_t {
		mv,
		inv,
		neg,
		neg_half,
		sign_extend,
		set_eq_zero,
		set_ne_zero,
		set_lt_zero,
		set_gt_zero,
		float_copy_sign,
		float_abs,
		float_neg,
		double_copy_sign,
		double_abs,
		double_neg,
		branch_eqz,
		branch_nez,
		branch_lez,
		branch_gez,
		branch_ltz,
		branch_gtz,
		jump,
		jump_and_link,
		jump_reg,
		jump_and_link_reg,
		ret,
		fence,

		csr_read_inst_retired,
		csr_read_cycles,
		csr_read_time,
		csr_read,
		csr_write,
		csr_set_bits,
		csr_clear_bits,
		csr_write_imm,
		csr_set_bits_imm,
		csr_clear_bits_imm,
		csr_float_read_status,
		csr_float_swap_status,
		csr_float_write_status,
		csr_float_read_rm,
		csr_float_swap_rm,
		csr_float_write_rm,
		csr_float_swap_rm_imm,
		csr_float_write_rm_imm,
		csr_float_read_exceptions,
		csr_float_swap_exceptions,
		csr_float_write_exceptions,
		csr_float_swap_exceptions_imm,
		csr_float_write_exceptions_imm,

		_invalid
	};

	namespace reg {
		static constexpr uint8_t Zero = 0;
		static constexpr uint8_t X1 = 1;
		static constexpr uint8_t X2 = 2;
		static constexpr uint8_t X5 = 5;
		static constexpr uint8_t X6 = 6;

		static constexpr uint8_t A0 = 10;
		static constexpr uint8_t A1 = 11;
		static constexpr uint8_t A2 = 12;
		static constexpr uint8_t A3 = 13;
		static constexpr uint8_t A4 = 14;
		static constexpr uint8_t A5 = 15;
		static constexpr uint8_t A6 = 16;
		static constexpr uint8_t A7 = 17;
	}

	namespace csr {
		/* read-and-write */
		static constexpr uint16_t fpExceptionFlags = 0x001;
		static constexpr uint16_t fpRoundingMode = 0x002;
		static constexpr uint16_t fpStatus = 0x003;

		/* read-only */
		static constexpr uint16_t cycles = 0xc00;
		static constexpr uint16_t realTime = 0xc01;
		static constexpr uint16_t instRetired = 0xc02;
	}

	struct Instruction {
	public:
		rv64::Opcode opcode = rv64::Opcode::_invalid;
		uint16_t misc = 0;
		rv64::Pseudo pseudo = rv64::Pseudo::_invalid;
		uint8_t dest = 0;
		uint8_t src1 = 0;
		uint8_t src2 = 0;
		uint8_t src3 = 0;
		uint8_t size = 0;
		int64_t imm = 0;

	public:
		constexpr bool isCall() const {
			/* check for the multi-instruction */
			if (opcode == rv64::Opcode::multi_call)
				return true;

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
		constexpr bool isMisaligned(env::guest_t address) const {
			if (opcode == rv64::Opcode::branch_eq || opcode == rv64::Opcode::branch_ne || opcode == rv64::Opcode::branch_ge_s ||
				opcode == rv64::Opcode::branch_ge_u || opcode == rv64::Opcode::branch_lt_s || opcode == rv64::Opcode::branch_lt_u)
				address += imm;
			else if (opcode == rv64::Opcode::jump_and_link_imm)
				address += imm;
			else
				return false;
			return ((address & 0x01) != 0);
		}
	};
}
