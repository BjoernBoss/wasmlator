/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"
#include "context-builder.h"

namespace gen::detail {
	class ContextWriter {
	private:
		const detail::ContextState& pState;

	public:
		ContextWriter(const detail::ContextState& state);

	private:
		void fCheckRange(uint32_t offset, gen::MemoryType type) const;
		void fMakeHostRead(uintptr_t host, gen::MemoryType type) const;
		void fMakeHostStartWrite(uintptr_t host) const;
		void fMakeHostEndWrite(gen::MemoryType type) const;

	public:
		void makeRead(uint32_t offset, gen::MemoryType type) const;
		void makeStartWrite(uint32_t offset, gen::MemoryType type) const;
		void makeEndWrite(gen::MemoryType type) const;
		void makeHostRead(const void* host, gen::MemoryType type) const;
		void makeStartHostWrite(void* host) const;
		void makeEndHostWrite(gen::MemoryType type) const;
		void makeTerminate(env::guest_t address) const;
		void makeNotDecodable(env::guest_t address) const;
		void makeNotReadable(env::guest_t address) const;
		void makeNotReachable(env::guest_t address) const;
	};
}
