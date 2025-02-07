#pragma once

#include "../gen-common.h"
#include "../block/gen-block.h"

namespace gen {
	enum class TraceType : uint8_t {
		none,
		block,
		chunk,
		instruction
	};

	namespace detail {
		struct GeneratorAccess {
			static bool Setup(gen::Generator& generator, std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, gen::TraceType trace, std::function<void(env::guest_t)> debugCheck);
			static void SetWriter(gen::Writer* writer);
			static gen::Translator* Get();
			static detail::BlockState* GetBlock();
			static bool CoreCreated();
			static void DebugCheck(env::guest_t address);
		};
	}

	class Generator {
		friend detail::GeneratorAccess;
	private:
		std::unique_ptr<gen::Translator> pTranslator;
		std::function<void(env::guest_t)> pDebugCheck;
		detail::BlockState pBlockState;
		wasm::Module* pModule = 0;
		wasm::Sink* pSink = 0;
		uint32_t pTranslationDepth = 0;
		gen::TraceType pTrace = gen::TraceType::none;

	public:
		Generator() = default;
		Generator(gen::Generator&&) = delete;
		Generator(const gen::Generator&) = delete;
		~Generator() = default;

	private:
		bool fSetup(std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, gen::TraceType trace, std::function<void(env::guest_t)> debugCheck);
		bool fFinalize();

	public:
		uint32_t translationDepth() const;
		gen::TraceType trace() const;
		bool debugCheck() const;
		wasm::Module* setModule(wasm::Module* mod);
		wasm::Sink* setSink(wasm::Sink* sink);
	};
}

template <> struct str::Formatter<gen::TraceType> {
	constexpr bool operator()(str::IsSink auto& sink, gen::TraceType val, std::u32string_view fmt) const {
		if (!fmt.empty())
			return false;
		switch (val) {
		case gen::TraceType::none:
			str::TranscodeAllTo(sink, U"None");
			break;
		case gen::TraceType::block:
			str::TranscodeAllTo(sink, U"Block");
			break;
		case gen::TraceType::chunk:
			str::TranscodeAllTo(sink, U"Chunk");
			break;
		case gen::TraceType::instruction:
			str::TranscodeAllTo(sink, U"Instruction");
			break;
		default:
			str::TranscodeAllTo(sink, U"%Unknown%");
			break;
		}
		return true;
	}
};
