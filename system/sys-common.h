#pragma once

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <unordered_set>

#include "../environment/environment.h"
#include "../generate/generate.h"
#include "../interface/logger.h"

namespace sys {
	class Debugger;

	enum class SyscallIndex : uint32_t {
		unknown,
		read,
		write,

		brk,

		getuid,
		geteuid,
		getgid,
		getegid,
	};
	struct UserSpaceSyscall {
	public:

	public:
		uint64_t args[6] = { 0 };
		uint64_t rawIndex = 0;
		sys::SyscallIndex index = sys::SyscallIndex::unknown;
	};

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
		*	Note: will call sys::Cpu::getSyscallArgs and sys::Cpu::setSyscallResult
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

		/* fetch the arguments for a unix syscall
		*	Note: will only be called in response to the ExecContext::syscall construction for userspace execution-contexts */
		virtual sys::UserSpaceSyscall getSyscallArgs() const = 0;

		/* set the result of the last syscall being performed
		*	Note: will only be called in response to the ExecContext::syscall construction for userspace execution-contexts */
		virtual void setSyscallResult(uint64_t value) = 0;

		/* fetch the name of all supported registers */
		virtual std::vector<std::u8string> queryNames() const = 0;

		/* decode the instruction at the address and return its size (size of null implicates decoding failure, may throw env::MemoryFault) */
		virtual std::pair<std::u8string, uint8_t> decode(uintptr_t address) const = 0;

		/* read the current cpu value (index matches the queried name-index) */
		virtual uintptr_t getValue(size_t index) const = 0;

		/* set a value of the current cpu state (index matches the queried name-index) */
		virtual void setValue(size_t index, uintptr_t value) = 0;

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
		/* fetch the current cpu implementation */
		virtual sys::Cpu* getCpu() const = 0;

		/* get the current pc */
		virtual env::guest_t getPC() const = 0;

		/* set the current pc */
		virtual void setPC(env::guest_t pc) = 0;

		/* continue execution until the debugger returns true for completed */
		virtual void run() = 0;
	};
}
