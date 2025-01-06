#pragma once

#include "../gen-common.h"

namespace gen {
	namespace detail {
		struct GeneratorAccess {
			static bool Setup(gen::Generator& generator, std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep);
			static void SetWriter(gen::Writer* writer);
			static gen::Translator* Get();
		};
	}

	class Generator {
		friend detail::GeneratorAccess;
	private:
		std::unique_ptr<gen::Translator> pTranslator;
		wasm::Module* pModule = 0;
		wasm::Sink* pSink = 0;
		uint32_t pTranslationDepth = 0;
		bool pSingleStep = false;

	public:
		Generator() = default;
		Generator(gen::Generator&&) = delete;
		Generator(const gen::Generator&) = delete;
		~Generator() = default;

	private:
		bool fSetup(std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep);

	public:
		uint32_t translationDepth() const;
		bool singleStep() const;
		wasm::Module* setModule(wasm::Module* mod);
		wasm::Sink* setSink(wasm::Sink* sink);
	};
}
