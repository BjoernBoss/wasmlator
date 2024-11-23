#pragma once

#include "../environment/environment.h"
#include "../generate/generate.h"

namespace sys {
	class Cpu : public gen::Translator {
	private:
		uint32_t pMemoryCaches = 0;
		uint32_t pContextSize = 0;

	protected:
		constexpr Cpu(uint32_t memoryCaches, uint32_t contextSize) : pMemoryCaches{ memoryCaches }, pContextSize{ contextSize } {}

	public:
		virtual void setupCore(wasm::Module& mod) = 0;
		virtual void setupContext(env::guest_t address) = 0;

	public:
		constexpr uint32_t memoryCaches() const {
			return pMemoryCaches;
		}
		constexpr uint32_t contextSize() const {
			return pContextSize;
		}
	};
}
