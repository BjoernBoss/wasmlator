#pragma once

#include "../env-common.h"

namespace env::detail {
	static constexpr uint32_t InternalCaches = 3;

	struct MemoryCache {
		env::guest_t address{ 0 };
		env::physical_t physical{ 0 };
		uint32_t size1{ 0 };
		uint32_t size2{ 0 };
		uint32_t size4{ 0 };
		uint32_t size8{ 0 };
	};

	class MemoryInteraction {
	private:
		uint32_t pCacheCount = 0;
		uint32_t pReadCache = 0;
		uint32_t pWriteCache = 0;
		uint32_t pCodeCache = 0;
		uint32_t pCacheAddress = 0;

	public:
		MemoryInteraction() = default;
		MemoryInteraction(detail::MemoryInteraction&&) = delete;
		MemoryInteraction(const detail::MemoryInteraction&) = delete;

	private:
		uint32_t fReadi32Fromi8(env::guest_t address) const;
		uint32_t fReadi32Fromu8(env::guest_t address) const;
		uint32_t fReadi32Fromi16(env::guest_t address) const;
		uint32_t fReadi32Fromu16(env::guest_t address) const;
		uint32_t fReadi32(env::guest_t address) const;
		uint64_t fReadi64(env::guest_t address) const;
		float fReadf32(env::guest_t address) const;
		double fReadf64(env::guest_t address) const;
		void fWritei32Fromi8(env::guest_t address, uint32_t value) const;
		void fWritei32Fromu8(env::guest_t address, uint32_t value) const;
		void fWritei32Fromi16(env::guest_t address, uint32_t value) const;
		void fWritei32Fromu16(env::guest_t address, uint32_t value) const;
		void fWritei32(env::guest_t address, uint32_t value) const;
		void fWritei64(env::guest_t address, uint64_t value) const;
		void fWritef32(env::guest_t address, float value) const;
		void fWritef64(env::guest_t address, double value) const;
		uint32_t fCodei32Fromi8(env::guest_t address) const;
		uint32_t fCodei32Fromu8(env::guest_t address) const;
		uint32_t fCodei32Fromi16(env::guest_t address) const;
		uint32_t fCodei32Fromu16(env::guest_t address) const;
		uint32_t fCodei32(env::guest_t address) const;
		uint64_t fCodei64(env::guest_t address) const;
		float fCodef32(env::guest_t address) const;
		double fCodef64(env::guest_t address) const;

	public:
		uint32_t configureAndAllocate(uint32_t address, uint32_t caches);
		uint32_t caches() const;
		uint32_t cacheAddress() const;
		uint32_t readCache() const;
		uint32_t writeCache() const;
		uint32_t codeCache() const;

	public:
		template <class Type>
		Type read(env::guest_t address) const {
			static_assert(std::is_arithmetic_v<Type>, "Can only read arithmetic types");
			if constexpr (std::is_same_v<Type, float>)
				return fReadf32(address);
			else if constexpr (std::is_same_v<Type, double>)
				return fReadf64(address);

			static constexpr bool sign = std::is_signed_v<Type>;
			switch (sizeof(Type)) {
			case 1:
				if constexpr (sign)
					return Type(fReadi32Fromi8(address));
				else
					return Type(fReadi32Fromu8(address));
			case 2:
				if constexpr (sign)
					return Type(fReadi32Fromi16(address));
				else
					return Type(fReadi32Fromu16(address));
			case 4:
				return Type(fReadi32(address));
			case 8:
			default:
				return Type(fReadi64(address));
			}
		}
		template <class Type>
		void write(env::guest_t address, Type value) {
			static_assert(std::is_arithmetic_v<Type>, "Can only write arithmetic types");
			if constexpr (std::is_same_v<Type, float>)
				fWritef32(address, value);
			else if constexpr (std::is_same_v<Type, double>)
				fWritef64(address, value);
			else {
				static constexpr bool sign = std::is_signed_v<Type>;
				switch (sizeof(Type)) {
				case 1:
					if constexpr (sign)
						fWritei32Fromi8(address, uint32_t(value));
					else
						fWritei32Fromu8(address, uint32_t(value));
					break;
				case 2:
					if constexpr (sign)
						fWritei32Fromi16(address, uint32_t(value));
					else
						fWritei32Fromu16(address, uint32_t(value));
					break;
				case 4:
					fWritei32(address, uint32_t(value));
					break;
				case 8:
				default:
					fWritei64(address, uint64_t(value));
					break;
				}
			}
		}
		template <class Type>
		Type code(env::guest_t address) const {
			static_assert(std::is_arithmetic_v<Type>, "Can only code-read arithmetic types");
			if constexpr (std::is_same_v<Type, float>)
				return fCodef32(address);
			else if constexpr (std::is_same_v<Type, double>)
				return fCodef64(address);

			static constexpr bool sign = std::is_signed_v<Type>;
			switch (sizeof(Type)) {
			case 1:
				if constexpr (sign)
					return Type(fCodei32Fromi8(address));
				else
					return Type(fCodei32Fromu8(address));
			case 2:
				if constexpr (sign)
					return Type(fCodei32Fromi16(address));
				else
					return Type(fCodei32Fromu16(address));
			case 4:
				return Type(fCodei32(address));
			case 8:
			default:
				return Type(fCodei64(address));
			}
		};
	};
}
