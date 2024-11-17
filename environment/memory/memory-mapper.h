#pragma once

#include "../env-common.h"

namespace env::detail {
	static constexpr uint32_t MinGrowthBytes = 32 * env::PageSize;
	static constexpr uint32_t ShiftMemoryFactor = 3;

	class MemoryMapper {
		static_assert(detail::PhysPageSize >= env::PageSize && (detail::PhysPageSize % env::PageSize) == 0, "The physical page size must a multiple of virtual page size");
	private:
		struct MemLookup {
			env::guest_t address = 0;
			detail::physical_t physical = 0;
			uint32_t size = 0;
		};
		struct MemPhysical {
			detail::physical_t physical = 0;
			uint32_t size = 0;
			bool used = false;
		};
		struct MemVirtual {
			env::guest_t address = 0;
			detail::physical_t physical = 0;
			uint32_t size = 0;
			uint32_t usage = 0;
		};

	private:
		std::vector<MemPhysical> pPhysical;
		std::vector<MemVirtual> pVirtual;
		mutable MemLookup pLastLookup;

	public:
		MemoryMapper() = default;
		MemoryMapper(detail::MemoryMapper&&) = delete;
		MemoryMapper(const detail::MemoryMapper&) = delete;

	private:
		size_t fLookupVirtual(env::guest_t address) const;
		size_t fLookupPhysical(detail::physical_t physical) const;

	private:
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

	public:
		void configure(uint32_t initialPageCount);

	public:
		void lookup(env::guest_t address, uint32_t size, uint32_t usage) const;
		const MemLookup& lastLookup() const;
		bool mmap(env::guest_t address, uint32_t size, uint32_t usage);
		void munmap(env::guest_t address, uint32_t size);
		void mprotect(env::guest_t address, uint32_t size, uint32_t usage);
	};
}
