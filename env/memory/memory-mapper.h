#pragma once

#include "memory-common.h"

namespace env::detail {
	class MemoryMapper {
		static_assert(env::PhysPageSize >= env::VirtPageSize && (env::PhysPageSize % env::VirtPageSize) == 0, "The physical page size must a multiple of virtual page size");
	private:
		struct MemLookup {
			env::guest_t address = 0;
			env::physical_t physical = 0;
			uint32_t size = 0;
		};
		struct MemPhysical {
			env::physical_t physical = 0;
			uint32_t size = 0;
			bool used = false;
		};
		struct MemVirtual {
			env::guest_t address = 0;
			env::physical_t physical = 0;
			uint32_t size = 0;
			uint32_t usage = 0;
		};

	private:
		env::Process* pProcess = 0;
		std::vector<MemPhysical> pPhysical;
		std::vector<MemVirtual> pVirtual;
		mutable MemLookup pLastLookup;

	public:
		MemoryMapper(env::Process* process, uint32_t initialAllocated);
		MemoryMapper(detail::MemoryMapper&&) = delete;
		MemoryMapper(const detail::MemoryMapper&) = delete;

	private:
		size_t fLookupVirtual(env::guest_t address) const;
		size_t fLookupPhysical(env::physical_t physical) const;

	private:
		uint32_t fExpandPhysical(uint32_t size, uint32_t growth) const;
		void fMovePhysical(env::physical_t dest, env::physical_t source, uint32_t size) const;
		void fFlushCaches() const;
		void fCheckConsistency() const;

	private:
		bool fMemExpandPrevious(size_t virt, env::guest_t address, uint32_t size, uint32_t usage);
		size_t fMemAllocatePhysical(uint32_t size, uint32_t growth);
		bool fMemAllocateIntermediate(size_t virt, uint32_t size, uint32_t usage);
		env::physical_t fMemMergePhysical(size_t virt, size_t phys, uint32_t size, size_t physPrev, size_t physNext);

	private:
		void fMemUnmapSingleBlock(size_t virt, env::guest_t address, uint32_t size);
		void fMemUnmapMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end);
		void fMemUnmapPhysical(size_t phys, uint32_t offset, uint32_t size);

	private:
		bool fMemProtectSingleBlock(size_t virt, env::guest_t address, uint32_t size, uint32_t usage);
		void fMemProtectMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end, uint32_t size, uint32_t usage);

	public:
		void setupCoreBody(wasm::Module& mod, env::CoreState& state) const;

	public:
		void lookup(env::guest_t address, uint32_t size, uint32_t usage) const;
		const MemLookup& lastLookup() const;
		bool mmap(env::guest_t address, uint32_t size, uint32_t usage);
		void munmap(env::guest_t address, uint32_t size);
		void mprotect(env::guest_t address, uint32_t size, uint32_t usage);
	};
}
