#pragma once

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <unordered_set>

#include "../environment/environment.h"
#include "../generate/generate.h"
#include "../interface/logger.h"
#include "syscall/sys-syscallable.h"
#include "debugger/sys-debuggable.h"

namespace sys {
	class CpuSyscallable;
	class CpuDebuggable;

	class ExecContext {
	private:
		bool pMultiThreaded = false;
		bool pUserspace = false;

	protected:
		constexpr ExecContext(bool multiThreaded, bool userspace) : pMultiThreaded{ multiThreaded }, pUserspace{ userspace } {}

	public:
		virtual ~ExecContext() = default;

	public:
		/* generate the code to perform a syscall (must not be called from non-userspace applications)
		*	Note: may abort the control-flow */
		virtual void syscall(env::guest_t address, env::guest_t nextAddress) = 0;

		/* generate the code to throw an exception (must not be called from non-userspace applications)
		*	Note: will abort the control-flow */
		virtual void throwException(uint64_t id, env::guest_t address, env::guest_t nextAddress) = 0;

		/* generate the code to flush the memory cache
		*	Note: may abort the control-flow */
		virtual void flushMemCache(env::guest_t address, env::guest_t nextAddress) = 0;

		/* generate the code to flush the instruction cache
		*	Note: will abort the control-flow */
		virtual void flushInstCache(env::guest_t address, env::guest_t nextAddress) = 0;

	public:
		constexpr bool multiThreaded() const {
			return pMultiThreaded;
		}
		constexpr bool userspace() const {
			return pUserspace;
		}
	};

	class Cpu : public gen::Translator {
	private:
		std::u8string pName;
		uint32_t pMemoryCaches = 0;
		uint32_t pContextSize = 0;

	protected:
		constexpr Cpu(std::u8string name, uint32_t memoryCaches, uint32_t contextSize) : pName{ name }, pMemoryCaches{ memoryCaches }, pContextSize{ contextSize } {}

	public:
		/* configure the cpu based on the given execution-context */
		virtual bool setupCpu(std::unique_ptr<sys::ExecContext>&& execContext) = 0;

		/* add any potential wasm-related functions to the core-module */
		virtual bool setupCore(wasm::Module& mod) = 0;

		/* configure the userspace-context with the given starting-execution address and stack-pointer address
		*	Note: will only be called for userspace execution-contexts */
		virtual bool setupContext(env::guest_t pcAddress, env::guest_t spAddress) = 0;

		/* convert the exception of the given id to a descriptive string */
		virtual std::u8string getExceptionText(uint64_t id) const = 0;

	public:
		/* provide access for userspace syscall interactions (return null if not supported)
		*	Note: will only be called for userspace execution-contexts */
		virtual std::unique_ptr<sys::CpuSyscallable> getSyscall() = 0;

		/* provide access for debugger interactions (return null if not supported) */
		virtual std::unique_ptr<sys::CpuDebuggable> getDebug() = 0;

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
	};
}
