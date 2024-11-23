#pragma once

#include "../env-common.h"
#include "context-access.h"
#include "context-bridge.h"

namespace env {
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
		void fTerminate(int32_t code, env::guest_t address);
		void fNotDecodable(env::guest_t address);
		void fNotReachable(env::guest_t address);

	public:
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
