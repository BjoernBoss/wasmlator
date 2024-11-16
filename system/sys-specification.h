#pragma once

#include <wasgen/wasm.h>
#include <cinttypes>

#include "../environment/env-common.h"
#include "../generate/translator/gen-writer.h"

namespace sys {
	/* wasm should only be created within setupCore/setupBlock,
	*	as they are wrapped to catch any potential wasm-issues */
	class Specification {
	private:
		size_t pMaxDepth = 0;
		uint32_t pMemoryCaches = 0;
		uint32_t pContextSize = 0;
		bool pIs32Bit = false;

	protected:
		constexpr Specification(size_t maxTranslationDepth, uint32_t memoryCaches, uint32_t contextSize, bool is32bit) : pMaxDepth{ maxTranslationDepth }, pMemoryCaches{ memoryCaches }, pContextSize{ contextSize }, pIs32Bit{ is32bit } {}

	public:
		virtual ~Specification() = default;

	public:
		virtual void setupCore(wasm::Module& mod) = 0;
		virtual std::vector<env::BlockExport> setupBlock(wasm::Module& mod) = 0;
		virtual void coreLoaded() = 0;
		virtual void blockLoaded() = 0;

	public:
		virtual void translationStarted() = 0;
		virtual void translationCompleted() = 0;
		virtual gen::Instruction fetchInstruction(env::guest_t address) = 0;
		virtual void produceInstructions(const gen::Writer& writer, const gen::Instruction* data, size_t count) = 0;

	public:
		constexpr size_t maxDepth() const {
			return pMaxDepth;
		}
		constexpr uint32_t memoryCaches() const {
			return pMemoryCaches;
		}
		constexpr uint32_t contextSize() const {
			return pContextSize;
		}
		constexpr bool is32bit() const {
			return pIs32Bit;
		}
	};
}
