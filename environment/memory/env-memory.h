/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
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
		struct MemoryVirtual {
			uint64_t physical = 0;
			uint64_t size = 0;
			uint32_t usage = 0;
		};
		struct MemoryPhysical {
			uint64_t size = 0;
			uint64_t users = 0;
		};
		struct MemoryFast {
			env::guest_t address{ 0 };
			uint32_t physical{ 0 };
			uint32_t size{ 0 };
			uint32_t usage = 0;
		};

		using MemVirtIt = std::map<env::guest_t, detail::MemoryVirtual>::iterator;
		using MemPhysIt = std::map<uint64_t, detail::MemoryPhysical>::iterator;

		static constexpr uint32_t MemoryFastCacheBits = 8;
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
		mutable std::map<env::guest_t, detail::MemoryVirtual> pVirtual;
		mutable std::map<uint64_t, detail::MemoryPhysical> pPhysical;
		uint64_t pPageSize = 0;
		uint64_t pPageBitShift = 0;
		uint32_t pCacheCount = 0;
		uint32_t pReadCache = 0;
		uint32_t pWriteCache = 0;
		uint32_t pCodeCache = 0;
		bool pDetectExecuteWrite = false;
		bool pXInvalidated = false;

	public:
		Memory() = default;
		Memory(env::Memory&&) = delete;
		Memory(const env::Memory&) = delete;

	private:
		detail::MemVirtIt fLookupVirtual(env::guest_t address) const;
		detail::MemPhysIt fLookupPhysical(uint64_t address) const;
		detail::MemoryLookup fConstructLookup(detail::MemVirtIt virt, uint32_t usage) const;
		detail::MemoryLookup fFastLookup(env::guest_t access, uint32_t usage) const;
		detail::MemoryLookup fCheckLookup(env::guest_t address, env::guest_t access, uint64_t size, uint32_t usage);

	private:
		uint64_t fPageOffset(env::guest_t address) const;
		uint64_t fExpandPhysical(uint64_t size, uint64_t growth) const;
		void fMovePhysical(uint64_t dest, uint64_t source, uint64_t size) const;
		void fFlushCaches();
		void fCheckConsistency() const;

	private:
		uint64_t fPhysEnd(detail::MemPhysIt phys) const;
		uint64_t fPhysEnd(detail::MemVirtIt virt) const;
		env::guest_t fVirtEnd(detail::MemVirtIt virt) const;

	private:
		detail::MemPhysIt fPhysSplit(detail::MemPhysIt phys, uint64_t address);
		detail::MemPhysIt fPhysMerge(detail::MemPhysIt phys);
		detail::MemVirtIt fVirtSplit(detail::MemVirtIt virt, env::guest_t address);
		detail::MemVirtIt fVirtMergePrev(detail::MemVirtIt virt);

	private:
		bool fMemExpandPrevious(detail::MemVirtIt prevVirt, uint64_t size, uint32_t usage);
		bool fMemAllocateIntermediate(detail::MemVirtIt prev, detail::MemVirtIt next, uint32_t usage);
		detail::MemPhysIt fMemAllocatePhysical(uint64_t size, uint64_t growth);
		uint64_t fMemMergePhysical(detail::MemVirtIt virt, detail::MemPhysIt phys, uint64_t size, detail::MemPhysIt physPrev, detail::MemPhysIt physNext);
		void fReducePhysical();
		bool fMMap(env::guest_t address, uint64_t size, uint32_t usage);

	private:
		void fCheckXInvalidated(env::guest_t address);
		void fCacheLookup(env::guest_t address, env::guest_t access, uint32_t size, uint32_t usage, uint32_t cache);
		uint64_t fRead(env::guest_t address, uint64_t size) const;
		void fWrite(env::guest_t address, uint64_t size, uint64_t value) const;
		uint64_t fCode(env::guest_t address, uint64_t size) const;

	public:
		uint64_t totalAllocated() const;
		uint64_t totalShared() const;
		uint64_t maxAllocate() const;
		void checkXInvalidated(env::guest_t address);
		std::pair<env::guest_t, uint64_t> findNext(env::guest_t address) const;
		uint32_t getUsage(env::guest_t address) const;

	public:
		env::guest_t alloc(uint64_t size, uint32_t usage);
		bool mmap(env::guest_t address, uint64_t size, uint32_t usage);
		bool munmap(env::guest_t address, uint64_t size);
		bool mprotect(env::guest_t address, uint64_t size, uint32_t usage);
		void mread(void* dest, env::guest_t source, uint64_t size, uint32_t usage);
		void mwrite(env::guest_t dest, const void* source, uint64_t size, uint32_t usage);
		void mclear(env::guest_t dest, uint64_t size, uint32_t usage);

	public:
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
