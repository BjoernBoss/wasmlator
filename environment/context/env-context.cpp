/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "env-context.h"

static util::Logger logger{ u8"env::context" };

void env::Context::fCheck(uint32_t size) const {
	if (size > uint32_t(pBuffer.size()))
		logger.fatal(u8"Cannot access [", size, u8"] bytes from context of size [", pBuffer.size(), u8']');
}

void env::Context::fTerminate(int32_t code, env::guest_t address) {
	throw env::Terminated{ address, code };
}
void env::Context::fCodeException(env::guest_t address, detail::CodeExceptions id) {
	switch (id) {
	case detail::CodeExceptions::notReachable:
		logger.fatal(u8"Execution reached address [", str::As{ U"#018x", address }, u8"] which was considered not reachable by super-block");
		break;
	case detail::CodeExceptions::notDecodable:
		throw env::Decoding{ address, false };
	case detail::CodeExceptions::notReadable:
		throw env::Decoding{ address, true };
	default:
		logger.fatal(u8"Unknown coding-exception [", size_t(id), u8"] encountered");
	}
}

void env::Context::terminate(int32_t code, env::guest_t address) {
	fTerminate(code, address);
}
