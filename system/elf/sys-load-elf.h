/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "sys-elf-types.h"

namespace sys::detail {
	struct ElfConfig {
		std::u8string interpreter;
		env::guest_t phAddress = 0;
		env::guest_t endOfData = 0;
		env::guest_t entry = 0;
		size_t phEntrySize = 0;
		size_t phOffset = 0;
		size_t phCount = 0;
		elf::MachineType machine = elf::MachineType::none;
		bool dynamic = false;
	};

	template <class Base>
	detail::ElfConfig ValidateElfHeader(const detail::Reader& reader);

	template <class Base>
	detail::ElfConfig ValidateElfLoadTyped(const detail::Reader& reader);

	template <class Base>
	env::guest_t LoadElfSingleProgHeader(env::guest_t baseAddress, size_t index, const detail::ProgramHeader<Base>& header, const detail::Reader& reader);

	template <class Base>
	void LoadElfProgHeadersTyped(env::guest_t baseAddress, detail::ElfConfig& config, const detail::Reader& reader);

	uint8_t CheckElfSignature(const detail::Reader& reader);

	detail::ElfConfig ValidateElfLoad(const detail::Reader& reader, uint8_t bitWidth);

	void LoadElfProgHeaders(env::guest_t baseAddress, detail::ElfConfig& config, const detail::Reader& reader, uint8_t bitWidth);
}
