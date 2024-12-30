#pragma once

#include "../gen-common.h"

namespace gen {
	namespace detail {
		struct GeneratorAccess {
			static bool Setup(gen::Generator& generator, std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep);
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
		const gen::Translator* translator() const;
		gen::Translator* translator();
		uint32_t translationDepth() const;
		bool singleStep() const;
		const wasm::Module& _module() const;
		wasm::Module& _module();
		const wasm::Sink& _sink() const;
		wasm::Sink& _sink();
		void setModule(wasm::Module* mod);
		void setSink(wasm::Sink* sink);
	};
}
