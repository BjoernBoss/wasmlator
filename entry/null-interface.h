/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../environment/environment.h"
#include "../generate/generate.h"

struct NullSystem final : public env::System {
	NullSystem() {}
	bool setupCore(wasm::Module& mod) final {
		return true;
	}
	void coreLoaded() final {}
	std::vector<env::BlockExport> setupBlock(wasm::Module& mod) final { return {}; }
	void blockLoaded() final {}
	void shutdown() final {}
};

struct NullTranslator final : public gen::Translator {
	NullTranslator() = default;
	void started(env::guest_t address) final {}
	void completed() final {}
	gen::Instruction fetch(env::guest_t address) final { return {}; }
	void produce(env::guest_t address, const uintptr_t* self, size_t count) final {}
};
