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
		constexpr uint32_t translationDepth() const {
			return pTranslationDepth;
		}
		constexpr bool singleStep() const {
			return pSingleStep;
		}
	};
}
