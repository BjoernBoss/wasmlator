/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"
#include "mapping-builder.h"

namespace gen::detail {
	class MappingWriter {
	private:
		const detail::MappingState& pState;

	public:
		MappingWriter(const detail::MappingState& state);

	public:
		void makeGetFunction() const;
		void makeDirectInvoke() const;
		void makeTailInvoke() const;
		const wasm::Prototype& blockPrototype() const;
	};
}
