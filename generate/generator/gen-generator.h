#pragma once

#include "../gen-common.h"
#include "../block/gen-block.h"

namespace gen {
	namespace detail {
		struct GeneratorAccess {
			static bool Setup(gen::Generator& generator, std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep, bool trace);
			static void SetWriter(gen::Writer* writer);
			static gen::Translator* Get();
			static detail::BlockState* GetBlock();
			static bool CoreCreated();
		};
	}

	class Generator {
		friend detail::GeneratorAccess;
	private:
		std::unique_ptr<gen::Translator> pTranslator;
		detail::BlockState pBlockState;
		wasm::Module* pModule = 0;
		wasm::Sink* pSink = 0;
		uint32_t pTranslationDepth = 0;
		bool pSingleStep = false;
		bool pTrace = false;

	public:
		Generator() = default;
		Generator(gen::Generator&&) = delete;
		Generator(const gen::Generator&) = delete;
		~Generator() = default;

	private:
		bool fSetup(std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep, bool trace);
		bool fFinalize();

	public:
		uint32_t translationDepth() const;
		bool singleStep() const;
		bool trace() const;
		wasm::Module* setModule(wasm::Module* mod);
		wasm::Sink* setSink(wasm::Sink* sink);
	};
}
