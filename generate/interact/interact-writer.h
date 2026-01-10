/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"
#include "interact-builder.h"

namespace gen::detail {
	class InteractWriter {
	private:
		const detail::InteractState& pState;

	public:
		InteractWriter(const detail::InteractState& state);

	public:
		void makeVoid(uint32_t index) const;
		void makeParam(uint32_t index) const;
	};
}
