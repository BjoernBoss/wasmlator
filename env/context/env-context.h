#pragma once

#include "../env-common.h"
#include "context-access.h"
#include "context-bridge.h"

namespace env {
	class Context {
		friend struct bridge::Context;
		friend class detail::ContextAccess;
	private:
		env::Process* pProcess = 0;
		uint32_t pSize = 0;
		uint32_t pAddress = 0;
		int32_t pExit = 0;

	public:
		Context(env::Process* process);
		Context(env::Context&&) = delete;
		Context(const env::Context&) = delete;

	private:
		void fRead(uint32_t offset, uint8_t* data, uint32_t size);
		void fWrite(uint32_t offset, const uint8_t* data, uint32_t size);
		void fSetExitCode(int32_t code);

	public:
		int32_t exitCode() const;
		template <class Type>
		Type read(uint32_t offset = 0) const {
			Type out{};
			fRead(offset, reinterpret_cast<uint8_t*>(&out), sizeof(Type));
			return out;
		}
		template <class Type>
		void write(const Type& value) {
			fWrite(0, reinterpret_cast<const uint8_t*>(&value), sizeof(Type));
		}
		template <class Type>
		void write(uint32_t offset, const Type& value) {
			fWrite(offset, reinterpret_cast<const uint8_t*>(&value), sizeof(Type));
		}
	};
}
