#pragma once

#include "../generate/generate.h"

#include "rv64-common.h"

namespace rv64 {
	namespace detail {
		/* writes value to stack */
		void LoadRegister(uint8_t reg, const gen::Writer& writer);

		/* expects value on stack but leaves it as a copy in the temporary variable */
		void StoreRegister(uint8_t reg, const gen::Writer& writer);

		void TranslateJAL(const gen::Instruction& gen, const rv64::Instruction& inst, const gen::Writer& writer);
	}

	/* performs primitive macro-expansion-like translation */
	void Translate(const gen::Instruction& gen, const rv64::Instruction& inst, const gen::Writer& writer);
}
