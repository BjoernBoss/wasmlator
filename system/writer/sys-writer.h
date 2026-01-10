/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "../sys-common.h"

namespace sys {
	class Writer {
		friend class sys::Userspace;
	private:
		struct {
			uint64_t id = 0;
			env::guest_t address = 0;
		} pException;
		struct {
			env::guest_t address = 0;
			env::guest_t next = 0;
		} pSyscall;
		struct {
			uint32_t flushInst = 0;
			uint32_t exception = 0;
			uint32_t syscall = 0;
		} pRegistered;

	private:
		Writer() = default;
		Writer(sys::Writer&&) = delete;
		Writer(const sys::Writer&) = delete;

	private:
		bool fSetup(detail::Syscall* syscall);

	public:
		/* generate the code to flush the memory cache
		*	Note: may abort the control-flow */
		void makeFlushMemCache(env::guest_t address, env::guest_t nextAddress) const;

		/* generate the code to flush the instruction cache
		*	Note: will abort the control-flow */
		void makeFlushInstCache(env::guest_t address, env::guest_t nextAddress) const;

		/* generate the code to perform a syscall
		*	Note: may abort the control-flow */
		void makeSyscall(env::guest_t address, env::guest_t nextAddress);

		/* generate the code to throw an exception
		*	Note: will abort the control-flow */
		void makeException(uint64_t id, env::guest_t address, env::guest_t nextAddress);
	};
}
