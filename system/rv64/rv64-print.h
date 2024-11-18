#pragma once

#include "rv64-common.h"
#include "rv64-decoder.h"

namespace rv64 {
	std::u8string ToString(const rv64::Instruction& inst);
}
