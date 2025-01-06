#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

#include "../environment/environment.h"
#include "../generate/generate.h"
#include "../interface/logger.h"

namespace sys {
	class Debugger;

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
		uint32_t pMemoryCaches = 0;
		uint32_t pContextSize = 0;

	protected:
		constexpr Cpu(uint32_t memoryCaches, uint32_t contextSize) : pMemoryCaches{ memoryCaches }, pContextSize{ contextSize } {}

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
		constexpr uint32_t memoryCaches() const {
			return pMemoryCaches;
		}
		constexpr uint32_t contextSize() const {
			return pContextSize;
		}
	};

	class Debuggable {
	protected:
		Debuggable() = default;

	public:
		virtual ~Debuggable() = default;

	public:
		/* read the current cpu state */
		virtual void getState(std::unordered_map<std::u8string, uintptr_t>& state) const = 0;

		/* get the current pc */
		virtual env::guest_t getPC() const = 0;

		/* decode the instruction at the address */
		virtual std::u8string decode(uintptr_t address) const = 0;

		/* set a value of the current cpu state */
		virtual void setValue(std::u8string_view name, uintptr_t value) = 0;

		/* set the current pc */
		virtual void setPC(env::guest_t pc) = 0;

		/* continue execution until the debugger returns true for completed */
		virtual void run() = 0;
	};
}
