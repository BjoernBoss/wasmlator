#pragma once

#include "gen-common.h"
#include "generator/gen-generator.h"
#include "core/gen-core.h"
#include "block/gen-block.h"
#include "block/gen-writer.h"
#include "glue/gen-glue.h"

namespace gen {
	/* Many generation operations require a generator instance to be configured.
	*	Note: shutdown must be performed through env::System::shutdown, and must only be exeucted from within external calls */
	gen::Generator* Instance();
	bool SetInstance(std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep, bool trace);
	void ClearInstance();

	/* maps to gen::Instance()->setModule() */
	extern wasm::Module* Module;

	/* maps to gen::Instance()->setSink() */
	extern wasm::Sink* Sink;

	/* access writer created by gen::Block */
	extern gen::Writer* Make;

	/* writes all instructions to gen::Instance()->setSink() */
	static struct {
		void operator[](const wasm::InstSimple& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstConst& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstOperand& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstWidth& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstMemory& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstTable& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstLocal& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstParam& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstGlobal& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstFunction& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstIndirect& inst) {
			(*gen::Sink)[inst];
		}
		void operator[](const wasm::InstBranch& inst) {
			(*gen::Sink)[inst];
		}
	} Add;
}
