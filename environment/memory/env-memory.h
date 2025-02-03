#pragma once

#include "../env-common.h"
#include "memory-access.h"
#include "memory-bridge.h"

namespace env {
	namespace detail {
		static constexpr uint64_t InitAllocPages = 64;
		static constexpr uint64_t MinGrowthPages = 32;
		static constexpr uint64_t ShiftMemoryFactor = 3;
		static constexpr uint32_t InternalCaches = 3;

		static constexpr env::guest_t MainAccessAddress = std::numeric_limits<env::guest_t>::max();

		/* start any allocations above the given start-address and use the given spacing
		*	between addresses, if possible - in order to allow growth of allocations (> 32gb) */
		static constexpr env::guest_t StartOfAllocations = 0x0000'4000'0000'0000;
		static constexpr env::guest_t EndOfAllocations = 0x0800'0000'0000'0000;
		static constexpr env::guest_t SpacingBetweenAllocations = 0x8'0000'0000;

		struct MemoryCache {
			env::guest_t address{ 0 };
			uint32_t physical{ 0 };
			uint32_t total{ 0 };
			uint32_t size1{ 0 };
			uint32_t size2{ 0 };
			uint32_t size4{ 0 };
			uint32_t size8{ 0 };
		};
		struct MemoryLookup {
			env::guest_t address = 0;
			uint64_t physical = 0;
			uint64_t size = 0;
		};
		struct MemoryPhysical {
			uint64_t size = 0;
			bool used = false;
		};
		struct MemoryVirtual {
			env::guest_t address = 0;
			uint64_t physical = 0;
			uint64_t size = 0;
			uint32_t usage = 0;
		};
		struct MemoryFast {
			env::guest_t address{ 0 };
			uint32_t physical{ 0 };
			uint32_t size{ 0 };
			uint32_t usage = 0;
		};

		using MemPhysIt = std::map<uint64_t, detail::MemoryPhysical>::iterator;

		static constexpr uint32_t MemoryFastCacheBits = 6;
		static constexpr uint32_t MemoryFastCount = (1 << detail::MemoryFastCacheBits);
		static constexpr uint32_t MemoryFastCacheConstRead = 31;
		static constexpr uint32_t MemoryFastCacheConstWrite = 37;
		static constexpr uint32_t MemoryFastCacheConstElse = 1;
	}

	class Memory {
		friend struct detail::MemoryBridge;
		friend struct detail::MemoryAccess;
	private:
		detail::MemoryFast pFastCache[detail::MemoryFastCount];
		mutable std::vector<detail::MemoryCache> pCaches;
		mutable std::map<uint64_t, detail::MemoryPhysical> pPhysical;
		std::vector<detail::MemoryVirtual> pVirtual;
		uint64_t pPageSize = 0;
		uint64_t pPageBitShift = 0;
		uint32_t pCacheCount = 0;
		uint32_t pReadCache = 0;
		uint32_t pWriteCache = 0;
		uint32_t pCodeCache = 0;

	public:
		Memory() = default;
		Memory(env::Memory&&) = delete;
		Memory(const env::Memory&) = delete;

	private:
		size_t fLookupVirtual(env::guest_t address) const;
		detail::MemPhysIt fLookupPhysical(uint64_t physical) const;
		detail::MemoryLookup fLookup(env::guest_t address, env::guest_t access, uint64_t size, uint32_t usage) const;

	private:
		uint64_t fPageOffset(env::guest_t address) const;
		uint64_t fExpandPhysical(uint64_t size, uint64_t growth) const;
		void fMovePhysical(uint64_t dest, uint64_t source, uint64_t size) const;
		void fFlushCaches();
		void fCheckConsistency() const;
		bool fCheckAllMapped(size_t virt, env::guest_t end) const;

	private:
		void fMemMakePhysicalUnused(detail::MemPhysIt phys);
		bool fMemExpandPrevious(size_t virt, env::guest_t address, uint64_t size, uint32_t usage);
		detail::MemPhysIt fMemAllocatePhysical(uint64_t size, uint64_t growth);
		bool fMemAllocateIntermediate(size_t virt, uint64_t size, uint32_t usage);
		uint64_t fMemMergePhysical(size_t virt, detail::MemPhysIt phys, uint64_t size, detail::MemPhysIt physPrev, detail::MemPhysIt physNext);
		bool fMMap(env::guest_t address, uint64_t size, uint32_t usage);

	private:
		void fMemUnmapSingleBlock(size_t virt, env::guest_t address, uint64_t size);
		bool fMemUnmapMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end);
		void fMemUnmapPhysical(detail::MemPhysIt phys, uint64_t offset, uint64_t size);

	private:
		void fMemProtectSingleBlock(size_t virt, env::guest_t address, uint64_t size, uint32_t usage);
		bool fMemProtectMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end, uint64_t size, uint32_t usage);

	private:
		void fCacheLookup(env::guest_t address, env::guest_t access, uint32_t size, uint32_t usage, uint32_t cache);
		uint64_t fRead(env::guest_t address, uint64_t size) const;
		void fWrite(env::guest_t address, uint64_t size, uint64_t value) const;
		uint64_t fCode(env::guest_t address, uint64_t size) const;

	public:
		env::guest_t alloc(uint64_t size, uint32_t usage);
		bool mmap(env::guest_t address, uint64_t size, uint32_t usage);
		bool munmap(env::guest_t address, uint64_t size);
		bool mprotect(env::guest_t address, uint64_t size, uint32_t usage);
		void mread(void* dest, env::guest_t source, uint64_t size, uint32_t usage) const;
		void mwrite(env::guest_t dest, const void* source, uint64_t size, uint32_t usage);
		void mclear(env::guest_t dest, uint64_t size, uint32_t usage);
		std::pair<env::guest_t, uint64_t> findNext(env::guest_t address) const;
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
