#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <memory>
#include <vector>

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
		uint64_t pc = 0;
	};
}
