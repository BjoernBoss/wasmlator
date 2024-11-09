#pragma once

#include "../env-common.h"
#include "memory-mapper.h"
#include "memory-access.h"
#include "memory-bridge.h"

namespace env {
	namespace detail {
		static constexpr uint32_t InitAllocBytes = 64 * env::VirtPageSize;
		static constexpr uint32_t InternalCaches = 3;

		struct MemoryCache {
			env::guest_t address{ 0 };
			env::physical_t physical{ 0 };
			uint32_t size1{ 0 };
			uint32_t size2{ 0 };
			uint32_t size4{ 0 };
			uint32_t size8{ 0 };
		};
	}

	class Memory {
		friend struct detail::MemoryBridge;
		friend struct detail::MemoryAccess;
	private:
		detail::MemoryMapper pMapper;
		uint32_t pCacheCount = 0;
		uint32_t pReadCache = 0;
		uint32_t pWriteCache = 0;
		uint32_t pCodeCache = 0;
		uint32_t pCacheAddress = 0;

	public:
		Memory() = default;
		Memory(env::Memory&&) = delete;
		Memory(const env::Memory&) = delete;

	private:
		uint64_t fRead(env::guest_t address, uint32_t size) const;
		void fWrite(env::guest_t address, uint32_t size, uint64_t value) const;
		uint64_t fCode(env::guest_t address, uint32_t size) const;

	public:
		bool mmap(env::guest_t address, uint32_t size, uint32_t usage);
		void munmap(env::guest_t address, uint32_t size);
		void mprotect(env::guest_t address, uint32_t size, uint32_t usage);
		template <class Type>
		Type read(env::guest_t address) const {
			static_assert(std::is_arithmetic_v<Type>, "Can only read arithmetic types");

			if constexpr (std::is_same_v<Type, float>)
				return std::bit_cast<float, uint32_t>(fRead(address, 4));
			else if constexpr (std::is_same_v<Type, double>)
				return std::bit_cast<double, uint64_t>(fRead(address, 8));
			return Type(fRead(address, sizeof(Type)));
		}
		template <class Type>
		void write(env::guest_t address, Type value) {
			static_assert(std::is_arithmetic_v<Type>, "Can only write arithmetic types");

			if constexpr (std::is_same_v<Type, float>)
				fWrite(address, 4, std::bit_cast<uint32_t, float>(value));
			else if constexpr (std::is_same_v<Type, double>)
				fWrite(address, 8, std::bit_cast<uint64_t, double>(value));
			else
				fWrite(address, sizeof(Type), value);
		}
		template <class Type>
		Type code(env::guest_t address) const {
			static_assert(std::is_arithmetic_v<Type>, "Can only code-read arithmetic types");

			if constexpr (std::is_same_v<Type, float>)
				return std::bit_cast<float, uint32_t>(fRead(address, 4));
			else if constexpr (std::is_same_v<Type, double>)
				return std::bit_cast<double, uint64_t>(fRead(address, 8));
			return Type(fRead(address, sizeof(Type)));
		};
	};
}
