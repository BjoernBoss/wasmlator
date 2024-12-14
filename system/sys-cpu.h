#pragma once

#include "../environment/environment.h"
#include "../generate/generate.h"
#include "sys-execcontext.h"

namespace sys {
	class Cpu : public gen::Translator {
	private:
		uint32_t pMemoryCaches = 0;
		uint32_t pContextSize = 0;

	protected:
		constexpr Cpu(uint32_t memoryCaches, uint32_t contextSize) : pMemoryCaches{ memoryCaches }, pContextSize{ contextSize } {}

	public:
		/* configure the cpu based on the given execution-context */
		virtual void setupCpu(std::unique_ptr<sys::ExecContext>&& execContext) = 0;

		/* add any potential wasm-related functions to the core-module */
		virtual void setupCore(wasm::Module& mod) = 0;

		/* configure the userspace-context with the given starting-execution address and stack-pointer address
		*	Note: will only be called for userspace execution-contexts */
		virtual void setupContext(env::guest_t pcAddress, env::guest_t spAddress) = 0;

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
}
