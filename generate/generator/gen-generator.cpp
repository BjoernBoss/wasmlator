#include "../generate.h"

static host::Logger logger{ u8"gen::generator" };

namespace global {
	static std::unique_ptr<gen::Generator> Instance;
}

wasm::Module* gen::Module = 0;
wasm::Sink* gen::Sink = 0;
gen::Writer* gen::Make = 0;

gen::Generator* gen::Instance() {
	return global::Instance.get();
}
bool gen::SetInstance(std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep) {
	if (global::Instance.get() != 0) {
		logger.error(u8"Cannot create generator as only one generator can exist at a time");
		return false;
	}

	/* setup the new instance */
	logger.log(u8"Creating new generator...");
	global::Instance = std::make_unique<gen::Generator>();

	/* configure the instance */
	if (detail::GeneratorAccess::Setup(*global::Instance.get(), std::move(translator), translationDepth, singleStep)) {
		logger.log(u8"Generator created");
		return true;
	}
	global::Instance.reset();
	return false;
}
void gen::ClearInstance() {
	logger.log(u8"Destroying generator...");
	global::Instance.reset();
	logger.log(u8"Generator destroyed");
}


bool gen::detail::GeneratorAccess::Setup(gen::Generator& generator, std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep) {
	return generator.fSetup(std::move(translator), translationDepth, singleStep);
}
void gen::detail::GeneratorAccess::SetWriter(gen::Writer* writer) {
	if (gen::Make != 0 && writer != 0)
		logger.fatal(u8"Generator can only be associated with one writer at a time");
	gen::Make = writer;
}
gen::Translator* gen::detail::GeneratorAccess::Get() {
	return gen::Instance()->pTranslator.get();
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
uint32_t gen::Generator::translationDepth() const {
	return pTranslationDepth;
}
bool gen::Generator::singleStep() const {
	return pSingleStep;
}
wasm::Module* gen::Generator::setModule(wasm::Module* mod) {
	if (mod != 0 && pModule != 0)
		logger.fatal(u8"Generator can only be associated with one module at a time");
	std::swap(pModule, mod);
	gen::Module = pModule;
	return mod;
}
wasm::Sink* gen::Generator::setSink(wasm::Sink* sink) {
	if (sink != 0 && pSink != 0)
		logger.fatal(u8"Generator can only be associated with one sink at a time");
	std::swap(pSink, sink);
	gen::Sink = pSink;
	return sink;
}
