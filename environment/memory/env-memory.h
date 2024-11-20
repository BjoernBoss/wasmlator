#pragma once

#include "../env-common.h"
#include "memory-access.h"
#include "memory-bridge.h"

namespace env {
	namespace detail {
		static constexpr uint32_t InitAllocPages = 64;
		static constexpr uint32_t InternalCaches = 3;
		static constexpr uint32_t MinGrowthPages = 32;
		static constexpr uint32_t ShiftMemoryFactor = 3;

		struct MemoryCache {
			env::guest_t address{ 0 };
			detail::physical_t physical{ 0 };
			uint32_t size1{ 0 };
			uint32_t size2{ 0 };
			uint32_t size4{ 0 };
			uint32_t size8{ 0 };
		};
		struct MemoryLookup {
			env::guest_t address = 0;
			detail::physical_t physical = 0;
			uint32_t size = 0;
		};
		struct MemoryPhysical {
			detail::physical_t physical = 0;
			uint32_t size = 0;
			bool used = false;
		};
		struct MemoryVirtual {
			env::guest_t address = 0;
			detail::physical_t physical = 0;
			uint32_t size = 0;
			uint32_t usage = 0;
		};
	}

	class Memory {
		friend struct detail::MemoryBridge;
		friend struct detail::MemoryAccess;
	private:

	private:
		mutable std::vector<detail::MemoryCache> pCaches;
		std::vector<detail::MemoryPhysical> pPhysical;
		std::vector<detail::MemoryVirtual> pVirtual;
		uint32_t pReadCache = 0;
		uint32_t pWriteCache = 0;
		uint32_t pCodeCache = 0;
		uint32_t pPageSize = 0;

	public:
		Memory() = default;
		Memory(env::Memory&&) = delete;
		Memory(const env::Memory&) = delete;

	private:
		size_t fLookupVirtual(env::guest_t address) const;
		size_t fLookupPhysical(detail::physical_t physical) const;
		detail::MemoryLookup fLookup(env::guest_t address, uint32_t size, uint32_t usage) const;

	private:
		uint32_t fPageOffset(env::guest_t address) const;
		uint32_t fExpandPhysical(uint32_t size, uint32_t growth) const;
		void fMovePhysical(detail::physical_t dest, detail::physical_t source, uint32_t size) const;
		void fFlushCaches() const;
		void fCheckConsistency() const;

	private:
		bool fMemExpandPrevious(size_t virt, env::guest_t address, uint32_t size, uint32_t usage);
		size_t fMemAllocatePhysical(uint32_t size, uint32_t growth);
		bool fMemAllocateIntermediate(size_t virt, uint32_t size, uint32_t usage);
		detail::physical_t fMemMergePhysical(size_t virt, size_t phys, uint32_t size, size_t physPrev, size_t physNext);

	private:
		void fMemUnmapSingleBlock(size_t virt, env::guest_t address, uint32_t size);
		void fMemUnmapMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end);
		void fMemUnmapPhysical(size_t phys, uint32_t offset, uint32_t size);

	private:
		bool fMemProtectSingleBlock(size_t virt, env::guest_t address, uint32_t size, uint32_t usage);
		void fMemProtectMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end, uint32_t size, uint32_t usage);

	private:
		void fCacheLookup(env::guest_t address, uint32_t size, uint32_t usage, uint32_t cache) const;
		uint64_t fRead(env::guest_t address, uint32_t size) const;
		void fWrite(env::guest_t address, uint32_t size, uint64_t value) const;
		uint64_t fCode(env::guest_t address, uint32_t size) const;

	public:
		bool mmap(env::guest_t address, uint32_t size, uint32_t usage);
		void munmap(env::guest_t address, uint32_t size);
		void mprotect(env::guest_t address, uint32_t size, uint32_t usage);
		void mread(uint8_t* dest, env::guest_t source, uint32_t size, uint32_t usage) const;
		void mwrite(env::guest_t dest, const uint8_t* source, uint32_t size, uint32_t usage);
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
				return std::bit_cast<float, uint32_t>(fCode(address, 4));
			else if constexpr (std::is_same_v<Type, double>)
				return std::bit_cast<double, uint64_t>(fCode(address, 8));
			return Type(fCode(address, sizeof(Type)));
		};
	};
}
