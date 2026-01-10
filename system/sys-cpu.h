/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "sys-common.h"

namespace sys {
	class Writer;

	class Cpu : public gen::Translator {
	private:
		std::u8string pName;
		uint32_t pMemoryCaches = 0;
		uint32_t pContextSize = 0;
		bool pMultiThreaded = false;
		bool pDetectWriteExecute = false;
		sys::ArchType pArchType = sys::ArchType::unknown;

	protected:
		constexpr Cpu(std::u8string name, uint32_t memoryCaches, uint32_t contextSize, bool multiThreaded, bool detectWriteExecute, sys::ArchType architecture) : pName{ name }, pMemoryCaches{ memoryCaches }, pContextSize{ contextSize }, pMultiThreaded{ multiThreaded }, pDetectWriteExecute{ detectWriteExecute }, pArchType{ architecture } {}

	public:
		/* configure the cpu to run in userspace mode */
		virtual bool setupCpu(sys::Writer* writer) = 0;

		/* add any potential wasm-related functions to the core-module */
		virtual bool setupCore(wasm::Module& mod) = 0;

		/* configure the context with the given starting-execution address and stack-pointer address */
		virtual bool setupContext(env::guest_t pcAddress, env::guest_t spAddress) = 0;

	public:
		/* fetch the arguments for a unix syscall
		*	Note: will only be called within code produced by sys::Writer
		*	Note: for self implemented syscalls, set result to arg[0] and set index to completed (must not throw any exceptions) */
		virtual sys::SyscallArgs syscallGetArgs() const = 0;

		/* set the result of the last syscall being performed
		*	Note: will always be called after fetching the syscall-arguments, before resuming the execution */
		virtual void syscallSetResult(uint64_t value) = 0;

		/* convert the exception of the given id to a descriptive string */
		virtual std::u8string getExceptionText(uint64_t id) const = 0;

	public:
		/* fetch the name of all supported registers */
		virtual std::vector<std::u8string> debugQueryNames() const = 0;

		/* decode the instruction at the address and return its size (size of null implicates decoding failure, may throw env::MemoryFault) */
		virtual std::pair<std::u8string, uint8_t> debugDecode(env::guest_t address) const = 0;

		/* read the current cpu value (index matches the queried name-index) */
		virtual uint64_t debugGetValue(size_t index) const = 0;

		/* set a value of the current cpu state (index matches the queried name-index) */
		virtual void debugSetValue(size_t index, uint64_t value) = 0;

	public:
		constexpr std::u8string name() const {
			return pName;
		}
		constexpr uint32_t memoryCaches() const {
			return pMemoryCaches;
		}
		constexpr uint32_t contextSize() const {
			return pContextSize;
		}
		constexpr bool multiThreaded() const {
			return pMultiThreaded;
		}
		constexpr bool detectWriteExecute() const {
			return pDetectWriteExecute;
		}
		constexpr sys::ArchType architecture() const {
			return pArchType;
		}
	};
}
