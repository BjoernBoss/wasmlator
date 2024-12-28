#include "../generate.h"

static host::Logger logger{ u8"gen::generator" };

namespace global {
	static std::unique_ptr<gen::Generator> Instance;
}

bool gen::detail::GeneratorAccess::Setup(gen::Generator& generator, std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep) {
	return generator.fSetup(std::move(translator), translationDepth, singleStep);
}

bool gen::Generator::fSetup(std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep) {
	pTranslator = std::move(translator);
	pTranslationDepth = translationDepth;
	pSingleStep = singleStep;

	/* log the new configuration */
	logger.info(u8"  Translation Depth: ", pTranslationDepth);
	logger.info(u8"  Single Step      : ", str::As{ U"S", pSingleStep });
	return true;
}
const gen::Translator* gen::Generator::translator() const {
	return pTranslator.get();
}
gen::Translator* gen::Generator::translator() {
	return pTranslator.get();
}


gen::Generator* gen::Instance() {
	return global::Instance.get();
}
bool gen::SetInstance(std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep) {
	if (global::Instance.get() != 0) {
		logger.fatal(u8"Cannot create generator as only one generator can exist at a time");
		return false;
	}

	/* allocate the new instance */
	logger.log(u8"Creating new generator...");
	global::Instance = std::make_unique<gen::Generator>();

	/* configure the instance */
	if (detail::GeneratorAccess::Setup(*global::Instance.get(), std::move(translator), translationDepth, singleStep)) {
		logger.log(u8"Generator created");
		return true;
	}
	global::Instance.reset();
	logger.warn(u8"Failed to setup generator");
	return false;
}
void gen::ClearInstance() {
	logger.log(u8"Destroying generator...");

	global::Instance.reset();
	logger.log(u8"Generator destroyed");
}
