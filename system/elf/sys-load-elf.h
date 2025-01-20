#pragma once

#include "sys-elf-types.h"

namespace sys::detail {
	struct ElfHeaderOut {
		env::guest_t start = 0;
		size_t phOffset = 0;
		size_t phCount = 0;
		size_t phSize = 0;
		bool dynamic = false;
	};

	template <class Base>
	detail::ElfHeaderOut ValidateElfHeader(const detail::ElfHeader<Base>* header, const detail::Reader& reader);

	template <class Base>
	env::guest_t CheckProgramHeaders(const detail::ElfHeaderOut& config, const detail::Reader& reader);
}
