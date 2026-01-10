/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "../env-common.h"
#include "context-access.h"
#include "context-bridge.h"

namespace env {
	namespace detail {
		enum class CodeExceptions : uint8_t {
			notDecodable,
			notReadable,
			notReachable
		};
	}

	class Context {
		friend struct detail::ContextBridge;
		friend struct detail::ContextAccess;
	private:
		std::vector<uint8_t> pBuffer;

	public:
		Context() = default;
		Context(env::Context&&) = delete;
		Context(const env::Context&) = delete;

	private:
		void fCheck(uint32_t size) const;

	private:
		void fTerminate(int32_t code, env::guest_t address);
		void fCodeException(env::guest_t address, detail::CodeExceptions id);

	public:
		void terminate(int32_t code, env::guest_t address);
		template <class Type>
		const Type& get() const {
			fCheck(sizeof(Type));
			return *reinterpret_cast<const Type*>(pBuffer.data());
		}
		template <class Type>
		Type& get() {
			fCheck(sizeof(Type));
			return *reinterpret_cast<Type*>(pBuffer.data());
		}
	};
}
