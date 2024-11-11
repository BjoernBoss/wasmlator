#pragma once

#include <wasgen/wasm.h>
#include <cinttypes>

#include "../generate/translator/gen-writer.h"

namespace sys {
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
		virtual void setupBlock(wasm::Module& mod) = 0;
		virtual void coreLoaded() = 0;
		virtual void bodyLoaded() = 0;

	public:
		virtual void blockStarted() = 0;
		virtual void blockCompleted() = 0;
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
