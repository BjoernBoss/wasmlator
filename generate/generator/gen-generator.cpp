/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include "../generate.h"

static util::Logger logger{ u8"gen::generator" };

namespace global {
	static std::unique_ptr<gen::Generator> Instance;
}

wasm::Module* gen::Module = 0;
wasm::Sink* gen::Sink = 0;
gen::Writer* gen::Make = 0;

gen::Generator* gen::Instance() {
	return global::Instance.get();
}
bool gen::SetInstance(std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, gen::TraceType trace, std::function<void(env::guest_t)> debugCheck) {
	if (global::Instance.get() != 0) {
		logger.error(u8"Cannot create generator as only one generator can exist at a time");
		return false;
	}

	/* setup the new instance */
	logger.log(u8"Creating new generator...");
	global::Instance = std::make_unique<gen::Generator>();

	/* configure the instance */
	if (detail::GeneratorAccess::Setup(*global::Instance.get(), std::move(translator), translationDepth, trace, debugCheck)) {
		logger.log(u8"Generator created");
		return true;
	}
	global::Instance.reset();
	return false;
}
void gen::ClearInstance() {
	logger.log(u8"Destroying generator...");

	/* also reset the global references, as an exception might have triggered
	*	the generator-cleanup, thereby not properly resetting the references */
	global::Instance.reset();
	gen::Module = 0;
	gen::Sink = 0;
	gen::Make = 0;
	logger.log(u8"Generator destroyed");
}


bool gen::detail::GeneratorAccess::Setup(gen::Generator& generator, std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, gen::TraceType trace, std::function<void(env::guest_t)> debugCheck) {
	return generator.fSetup(std::move(translator), translationDepth, trace, debugCheck);
}
void gen::detail::GeneratorAccess::SetWriter(gen::Writer* writer) {
	if (gen::Make != 0 && writer != 0)
		logger.fatal(u8"Generator can only be associated with one writer at a time");
	gen::Make = writer;
}
gen::Translator* gen::detail::GeneratorAccess::Get() {
	return gen::Instance()->pTranslator.get();
}
gen::detail::BlockState* gen::detail::GeneratorAccess::GetBlock() {
	return &global::Instance->pBlockState;
}
bool gen::detail::GeneratorAccess::CoreCreated() {
	return global::Instance->fFinalize();
}
void gen::detail::GeneratorAccess::DebugCheck(env::guest_t address) {
	global::Instance->pDebugCheck(address);
}


bool gen::Generator::fSetup(std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, gen::TraceType trace, std::function<void(env::guest_t)> debugCheck) {
	pTranslator = std::move(translator);
	pTranslationDepth = translationDepth;
	pDebugCheck = debugCheck;
	pTrace = trace;

	/* check if a debugger is attached, in which case the translation-depth should be reset to 0 */
	if (bool(pDebugCheck))
		pTranslationDepth = 0;

	/* log the new configuration */
	logger.info(u8"  Translation Depth: ", pTranslationDepth);
	logger.info(u8"  Trace            : ", pTrace);
	logger.info(u8"  Debug Check      : ", str::As{ U"S", bool(pDebugCheck) });

	/* check if a trace-callback needs to be registered */

	return true;
}
bool gen::Generator::fFinalize() {
	return detail::BlockAccess::Setup(pBlockState);
}
uint32_t gen::Generator::translationDepth() const {
	return pTranslationDepth;
}
gen::TraceType gen::Generator::trace() const {
	return pTrace;
}
bool gen::Generator::debugCheck() const {
	return bool(pDebugCheck);
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
