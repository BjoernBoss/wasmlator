#pragma once

#include "env-memory-common.h"

namespace env::detail {
	class MemoryMapper {
		friend struct bridge::Memory;
	private:
		struct MemLookup {
			env::addr_t address{ 0 };
			env::physical_t physical{ 0 };
			uint32_t size{ 0 };
		};
		struct MemPhysical {
			env::physical_t physical = 0;
			uint32_t size = 0;
			bool used = false;
		};
		struct MemVirtual {
			env::addr_t address = 0;
			env::physical_t physical = 0;
			uint32_t size = 0;
			uint32_t usage = 0;
		};

	private:
		env::Context* pContext{ 0 };
		std::vector<MemPhysical> pPhysical;
		std::vector<MemVirtual> pVirtual;
		mutable MemLookup pLastLookup;

	public:
		MemoryMapper(env::Context& context);
		MemoryMapper(detail::MemoryMapper&&) = delete;
		MemoryMapper(const detail::MemoryMapper&) = delete;

	private:
		size_t fLookupVirtual(env::addr_t address) const;
		size_t fLookupPhysical(env::physical_t physical) const;

	private:
		bool fExpandPhysical(uint32_t size) const;
		void fMovePhysical(env::physical_t dest, env::physical_t source, uint32_t size) const;
		void fFlushCaches() const;

	private:
		int8_t fMemExpandPrevious(size_t virt, env::addr_t address, uint32_t size, uint32_t usage);
		size_t fMemAllocatePhysical(uint32_t size, uint32_t growth);
		env::physical_t fMemMergePhysical(size_t virt, size_t phys, uint32_t size, bool hasPrev, bool hasNext);

	public:
		void lookup(env::addr_t address, uint32_t size, uint32_t usage) const;
		const MemLookup& lastLookup() const;
		bool mmap(env::addr_t address, uint32_t size, uint32_t usage);
		void munmap(env::addr_t address, uint32_t size);
		void mprotect(env::addr_t address, uint32_t size, uint32_t usage);
	};
}
