/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include "env-memory.h"

static util::Logger logger{ u8"env::memory" };

env::detail::MemVirtIt env::Memory::fLookupVirtual(env::guest_t address) const {
	env::detail::MemVirtIt it = pVirtual.upper_bound(address);

	/* check if no entry is greater than the physical address */
	if (it == pVirtual.begin())
		return pVirtual.end();
	--it;

	/* check if the range contains the given address (cannot be equal to end) */
	if (it->first + it->second.size <= address)
		return pVirtual.end();
	return it;
}
env::detail::MemPhysIt env::Memory::fLookupPhysical(uint64_t address) const {
	env::detail::MemPhysIt it = pPhysical.upper_bound(address);

	/* check if no entry is greater than the physical address */
	if (it == pPhysical.begin())
		return pPhysical.end();
	--it;

	/* check if the range contains the given address (cannot be equal to end) */
	if (it->first + it->second.size <= address)
		return pPhysical.end();
	return it;
}
env::detail::MemoryLookup env::Memory::fConstructLookup(detail::MemVirtIt virt, uint32_t usage) const {
	detail::MemoryLookup lookup = detail::MemoryLookup{ virt->first, virt->second.physical, virt->second.size };

	/* check if executable pages should not be considered */
	bool skipExecutable = (pDetectExecuteWrite && (usage & env::Usage::Write) == env::Usage::Write);

	/* collect all previous contiguous regions of the same usage */
	for (detail::MemVirtIt it = virt; it != pVirtual.begin();) {
		--it;
		if (fVirtEnd(it) != lookup.address || fPhysEnd(it) != lookup.physical || (it->second.usage & usage) != usage)
			break;
		if (skipExecutable && (it->second.usage & env::Usage::Execute) == env::Usage::Execute)
			break;
		lookup = { it->first, it->second.physical, it->second.size + lookup.size };
	}

	/* collect all upcoming contiguous regions of the same usage */
	for (detail::MemVirtIt it = std::next(virt); it != pVirtual.end(); ++it) {
		if (lookup.address + lookup.size != it->first || lookup.physical + lookup.size != it->second.physical || (it->second.usage & usage) != usage)
			break;
		if (skipExecutable && (it->second.usage & env::Usage::Execute) == env::Usage::Execute)
			break;
		lookup.size += it->second.size;
	}
	return lookup;
}
env::detail::MemoryLookup env::Memory::fFastLookup(env::guest_t access, uint32_t usage) const {
	/* lookup the virtual mapping containing the corresponding accessed-address (must exist, as fast-lookup requires a previous checked lookup) */
	env::detail::MemVirtIt virt = fLookupVirtual(access);
	return fConstructLookup(virt, usage);
}
env::detail::MemoryLookup env::Memory::fCheckLookup(env::guest_t address, env::guest_t access, uint64_t size, uint32_t usage) {
	/* lookup the virtual mapping containing the corresponding accessed-address */
	env::detail::MemVirtIt virt = fLookupVirtual(access);
	if (virt == pVirtual.end())
		throw env::MemoryFault{ address, access, size, usage, 0 };

	/* check if the usage attributes are valid */
	if ((virt->second.usage & usage) != usage)
		throw env::MemoryFault{ address, access, size, usage, virt->second.usage };

	/* check if checks for writes to executables must be performed */
	bool checkWriteToExecutable = (pDetectExecuteWrite && (usage & env::Usage::Write) == env::Usage::Write);

	/* traverse over the virtual pages and check if they are all mapped properly and if a write to an executable page is being performed */
	uint64_t current = virt->first, end = access + size;
	for (detail::MemVirtIt it = virt; current < end; ++it) {
		if (it == pVirtual.end() || current != it->first)
			throw env::MemoryFault{ address, access, size, usage, 0 };
		if ((it->second.usage & usage) != usage)
			throw env::MemoryFault{ address, access, size, usage, it->second.usage };
		current = fVirtEnd(it);

		/* check if the operation performs a write to an executable page */
		if (checkWriteToExecutable && (it->second.usage & env::Usage::Execute) == env::Usage::Execute)
			pXInvalidated = true;
	}

	/* return the final contiguous lookup */
	return fConstructLookup(virt, usage);
}

uint64_t env::Memory::fPageOffset(env::guest_t address) const {
	return (address & (pPageSize - 1));
}
uint64_t env::Memory::fExpandPhysical(uint64_t size, uint64_t growth) const {
	/* check to prevent overflows */
	uint64_t firstAlloc = size + growth;
	if (firstAlloc < size)
		return 0;

	/* allocate a little bit more to reduce the number of growings */
	uint64_t pages = detail::PhysPageCount(std::max<uint64_t>(detail::MinGrowthPages * pPageSize, firstAlloc));
	if (!detail::MemoryBridge::ExpandPhysical(pages)) {
		uint64_t exact = detail::PhysPageCount(size);
		if (exact >= pages || !detail::MemoryBridge::ExpandPhysical(exact))
			return 0;
		pages = exact;
	}

	/* notify about the grown memory */
	uint64_t total = fPhysEnd(std::prev(pPhysical.end())) + pages * detail::PhysPageSize;
	logger.info(u8"Memory expanded to: ", str::As{ U"#018x", total });
	host::HostCheckMetrics();
	return (pages * detail::PhysPageSize);
}
void env::Memory::fMovePhysical(uint64_t dest, uint64_t source, uint64_t size) const {
	detail::MemoryBridge::MovePhysical(dest, source, size);
}
void env::Memory::fFlushCaches() {
	fCheckConsistency();

	/* flush the actual caches and fast-caches */
	logger.trace(u8"Flushing caches");
	std::memset(pCaches.data(), 0, sizeof(detail::MemoryCache) * pCaches.size());
	std::memset(pFastCache, 0, sizeof(detail::MemoryFast) * detail::MemoryFastCount);
}
void env::Memory::fCheckConsistency() const {
	/* physical mapping can never be empty */
	if (pPhysical.empty())
		logger.fatal(u8"Physical slots invalid");

	/* neighboring physical slots must always be merged if possible and must be non-empty and contiguous */
	uint64_t physAddress = 0, totalPhysUsed = 0, physLastUsers = 0;
	for (const auto& [address, phys] : pPhysical) {
		totalPhysUsed += phys.users * phys.size;

		if (phys.size == 0 || fPageOffset(phys.size) != 0)
			logger.fatal(u8"Physical slot [", str::As{ U"#018x", address }, u8"] size is invalid");
		if (address != physAddress)
			logger.fatal(u8"Physical slot [", str::As{ U"#018x", address }, u8"] address is invalid");
		if (physAddress > 0 && physLastUsers == phys.users)
			logger.fatal(u8"Physical slot [", str::As{ U"#018x", address }, u8"] users is invalid");
		physLastUsers = phys.users;
		physAddress += phys.size;
	}

	/* neighboring virtual slots must always be merged if usage is identical and must be non-empty */
	uint64_t totalVirtUsed = 0, virtAddress = 0;
	uint32_t virtLastUsage = 0;
	for (const auto& [address, virt] : pVirtual) {
		detail::MemPhysIt phys = fLookupPhysical(virt.physical);

		/* ensure the entire range is mapped to physical memory */
		if (phys == pPhysical.end())
			logger.fatal(u8"Virtual slot [", str::As{ U"#018x", address }, u8"] physical is invalid");
		uint64_t remaining = virt.size + (virt.physical - phys->first);
		while (true) {
			if (phys->second.users > 0) {
				if (phys->second.size >= remaining)
					break;
				remaining -= phys->second.size;
				if (++phys != pPhysical.end())
					continue;
			}
			logger.fatal(u8"Virtual slot [", str::As{ U"#018x", address }, u8"] physical not fully mapped");
		}

		/* validate the slot itself */
		if (virt.size == 0 || fPageOffset(virt.size) != 0)
			logger.fatal(u8"Virtual slot [", str::As{ U"#018x", address }, u8"] size is invalid");
		if (totalVirtUsed > 0 && virtLastUsage == virt.usage && virtAddress == address)
			logger.fatal(u8"Virtual slot [", str::As{ U"#018x", address }, u8"] usage is invalid");
		if (virtAddress > address)
			logger.fatal(u8"Virtual slot [", str::As{ U"#018x", address }, u8"] address is invalid");

		virtAddress = address + virt.size;
		totalVirtUsed += virt.size;
		virtLastUsage = virt.usage;
	}

	if (totalPhysUsed != totalVirtUsed)
		logger.fatal(u8"Phyiscal used memory does not match virtual used memory");
	logger.debug(u8"Memory consistency-check performed");
}

uint64_t env::Memory::fPhysEnd(detail::MemPhysIt phys) const {
	return phys->first + phys->second.size;
}
uint64_t env::Memory::fPhysEnd(detail::MemVirtIt virt) const {
	return virt->second.physical + virt->second.size;
}
env::guest_t env::Memory::fVirtEnd(detail::MemVirtIt virt) const {
	return virt->first + virt->second.size;
}

env::detail::MemPhysIt env::Memory::fPhysSplit(detail::MemPhysIt phys, uint64_t address) {
	uint64_t size = (address - phys->first);

	/* insert the new next block */
	detail::MemPhysIt next = pPhysical.insert(std::next(phys), { address, detail::MemoryPhysical{ phys->second.size - size, phys->second.users } });

	/* reduce the size of the current block */
	phys->second.size = size;
	return next;
}
env::detail::MemPhysIt env::Memory::fPhysMerge(detail::MemPhysIt phys) {
	/* check if the next entry can be removed */
	detail::MemPhysIt next = std::next(phys);
	if (next != pPhysical.end() && next->second.users == phys->second.users) {
		phys->second.size += next->second.size;
		pPhysical.erase(next);
	}

	/* check if the previous entry can be removed */
	next = phys;
	if (phys != pPhysical.begin() && (--phys)->second.users == next->second.users) {
		phys->second.size += next->second.size;
		pPhysical.erase(next);
	}
	return phys;
}
env::detail::MemVirtIt env::Memory::fVirtSplit(detail::MemVirtIt virt, env::guest_t address) {
	uint64_t size = (address - virt->first);

	/* insert the new next block */
	detail::MemVirtIt next = pVirtual.insert(std::next(virt), { address, detail::MemoryVirtual{ virt->second.physical + size, virt->second.size - size, virt->second.usage } });

	/* reduce the size of the current block */
	virt->second.size = size;
	return next;
}
env::detail::MemVirtIt env::Memory::fVirtMergePrev(detail::MemVirtIt virt) {
	if (virt == pVirtual.begin())
		return virt;

	/* check if the current entry can be removed */
	detail::MemVirtIt prev = std::prev(virt);
	if (fVirtEnd(prev) != virt->first || prev->second.usage != virt->second.usage)
		return virt;
	if (fPhysEnd(prev) != virt->second.physical)
		return virt;

	/* merge the blocks and remove the current entry */
	prev->second.size += virt->second.size;
	pVirtual.erase(virt);
	return prev;
}

bool env::Memory::fMemExpandPrevious(detail::MemVirtIt prevVirt, uint64_t size, uint32_t usage) {
	detail::MemPhysIt phys = fLookupPhysical(prevVirt->second.physical);

	/* check if the allocation aligns with the physical block as the physical block is otherwise shared with other virtual mappings */
	if (fPhysEnd(prevVirt) != fPhysEnd(phys))
		return false;

	/* check if the next page is already used, in which case no expansion is possible */
	detail::MemPhysIt next = std::next(phys);
	if (next != pPhysical.end() && next->second.users > 0)
		return false;
	uint64_t allocated = 0;

	/* check if the next physical page can be consumed */
	if (next != pPhysical.end() && next->second.size >= size) {
		allocated = next->second.size;
		pPhysical.erase(next);
	}

	/* expand the physical memory at the end */
	else {
		/* check if the physical page is at the end */
		if (next != pPhysical.end() && std::next(next) != pPhysical.end())
			return false;

		/* try to expand the physical memory (may allocate more than the needed amount) */
		uint64_t needed = size - (next != pPhysical.end() ? next->second.size : 0);
		allocated = fExpandPhysical(needed, size);
		if (allocated == 0)
			return false;

		/* merge the size of the next unused block with the current unused block */
		if (next != pPhysical.end()) {
			allocated += next->second.size;
			pPhysical.erase(next);
		}
	}

	/* update the physical slot to contain the entire allocated space and check if sections need to be broken off */
	phys->second.size += allocated;
	if (allocated > size)
		fPhysSplit(phys, fPhysEnd(phys) - (allocated - size))->second.users = 0;
	if (phys->second.users > 1)
		fPhysSplit(phys, fPhysEnd(phys) - size)->second.users = 1;

	/* update the virtual slot */
	uint64_t physical = fPhysEnd(prevVirt);
	prevVirt->second.size += size;
	if (prevVirt->second.usage != usage)
		fVirtSplit(prevVirt, fVirtEnd(prevVirt) - size)->second.usage = usage;

	/* clear the next allocated block of memory */
	detail::MemoryBridge::ClearPhysical(physical, size);
	return true;
}
bool env::Memory::fMemAllocateIntermediate(detail::MemVirtIt prev, detail::MemVirtIt next, uint32_t usage) {
	uint64_t inbetween = next->first - fVirtEnd(prev);

	/* check if the previous and next block already lie aligned in physical memory */
	if (fPhysEnd(prev) + inbetween != next->second.physical)
		return false;

	/* check if they both lie in the same physical block, in which case the intermediate range must already be allocated */
	detail::MemPhysIt phys = fLookupPhysical(prev->second.physical);
	if (fPhysEnd(phys) > next->second.physical)
		return false;

	/* check if the intermediate physical block is not yet allocated (next block
	*	must exist, as the upcoming virtual entry is at a higher address) */
	++phys;
	if (phys->second.users > 0 || phys->second.size != inbetween)
		return false;

	/* clear the next allocated block of memory */
	detail::MemoryBridge::ClearPhysical(phys->first, phys->second.size);

	/* merge the three physical ranges back together */
	phys->second.users = 1;
	fPhysMerge(phys);

	/* update the previous virtual block to contain the new allocation */
	prev->second.size += inbetween;
	if (prev->second.usage != usage)
		fVirtSplit(prev, fVirtEnd(prev) - inbetween)->second.usage = usage;

	/* try to merge the next virtual block with the current virtual block */
	fVirtMergePrev(next);
	return true;
}
env::detail::MemPhysIt env::Memory::fMemAllocatePhysical(uint64_t size, uint64_t growth) {
	detail::MemPhysIt phys = pPhysical.begin();

	/* look for the next region large enough to store the required memory */
	for (; phys != pPhysical.end(); ++phys) {
		if (phys->second.users == 0 && phys->second.size >= size)
			break;
	}

	/* check if the physical memory needs to be allocated */
	if (phys == pPhysical.end()) {
		uint64_t allocate = fExpandPhysical(size, growth);
		if (allocate == 0)
			return phys;

		/* add the newly allocated physical memory/slot (physical map can never be empty,
		*	as it will always be initialized with at least some unused memory) */
		(--phys)->second.size += allocate;
		if (phys->second.users > 0)
			(phys = fPhysSplit(phys, fPhysEnd(phys) - allocate))->second.users = 0;
	}
	return phys;
}
uint64_t env::Memory::fMemMergePhysical(detail::MemVirtIt virt, detail::MemPhysIt phys, uint64_t size, detail::MemPhysIt physPrev, detail::MemPhysIt physNext) {
	uint64_t total = size + (physPrev != pPhysical.end() ? physPrev->second.size : 0) + (physNext != pPhysical.end() ? physNext->second.size : 0);

	/* ensure the physical entry is only as large as necessary and extract the actual physical address */
	if (phys->second.size > total)
		fPhysSplit(phys, phys->first + total);
	uint64_t physical = phys->first + (physPrev == pPhysical.end() ? 0 : physPrev->second.size);

	/* move the upper range into place and release the original blocks */
	if (physNext != pPhysical.end()) {
		uint64_t address = fPhysEnd(phys) - physNext->second.size;
		fMovePhysical(address, physNext->first, physNext->second.size);

		/* patch the physical address of all moved virtual blocks */
		detail::MemVirtIt it = virt;
		while (true) {
			(++it)->second.physical = address;
			address += it->second.size;
			if (fPhysEnd(it) == fPhysEnd(phys))
				break;
		}

		/* release the orignal physical block (previous users can at most have been one) */
		physNext->second.users = 0;
		fPhysMerge(physNext);
	}

	/* move the lower range into place and release the original blocks */
	if (physPrev != pPhysical.end()) {
		uint64_t address = phys->first + physPrev->second.size;
		fMovePhysical(phys->first, physPrev->first, physPrev->second.size);
		std::prev(virt)->second.physical = phys->first;

		/* patch the physical address of all moved virtual blocks */
		detail::MemVirtIt it = virt;
		while (true) {
			address -= (--it)->second.size;
			it->second.physical = address;
			if (address == phys->first)
				break;
		}

		/* release the orignal physical block (previous users can at most have been one) */
		physPrev->second.users = 0;
		fPhysMerge(physPrev);
	}

	/* mark the physical page as used and merge it with neighbors */
	phys->second.users = 1;
	fPhysMerge(phys);
	return physical;
}
void env::Memory::fReducePhysical() {
	/* check if the lowest physical memory should be reduced and moved down
	*	(there must at least be one slot, as an allocation has occurred in this call) */
	uint64_t end = fPhysEnd(std::prev(pPhysical.end()));
	if (pPhysical.begin()->second.users > 0 || pPhysical.begin()->second.size < (end / detail::ShiftMemoryFactor))
		return;

	/* move the physical memory down */
	uint64_t shift = pPhysical.begin()->second.size;
	fMovePhysical(0, shift, end - shift);

	/* shift the virtual slots down */
	for (auto& [address, virt] : pVirtual)
		virt.physical -= shift;

	/* move the physical memory down */
	detail::MemPhysIt it = pPhysical.erase(pPhysical.begin());
	uint64_t address = 0;
	while (it != pPhysical.end()) {
		pPhysical.insert(it, { address, it->second });
		address += it->second.size;
		it = pPhysical.erase(it);
	}
}
bool env::Memory::fMMap(env::guest_t address, uint64_t size, uint32_t usage) {
	/* Note: no need to detect for x-flag changes for flushing as the memory is newly allocated and can therefore not alter any previous state */
	logger.fmtDebug(u8"Mapping [{:#018x}] with size [{:#010x}] and usage [{}]", address, size, env::Usage::Print{ usage });

	/* check if the address and size are aligned properly and validate the usage */
	if (fPageOffset(address) != 0 || fPageOffset(size) != 0 || size == 0) {
		logger.error(u8"Mapping requires address and size to be page-aligned and size greater than zero");
		return false;
	}
	if ((usage & ~env::Usage::Usages) != 0) {
		logger.error(u8"Memory-usage must only consist of read/write/execute/lock usage");
		return false;
	}

	/* fetch the previous and next virtual range, and ensure that the range does not overlap existing mappings */
	detail::MemVirtIt next = pVirtual.upper_bound(address);
	detail::MemVirtIt prev = (next == pVirtual.begin() ? pVirtual.end() : std::prev(next));
	if ((next != pVirtual.end() && next->first - address < size) || (prev != pVirtual.end() && fVirtEnd(prev) > address)) {
		logger.error(u8"Mapping range is already partially mapped");
		return false;
	}

	/* check if the previous or next iterators are direct neighbors */
	bool directPrev = (prev != pVirtual.end() && fVirtEnd(prev) == address);
	bool directNext = (next != pVirtual.end() && next->first == address + size);

	/* check if an existing neighboring region can just be expanded or if both neighbors already align (edge-case) */
	if (directPrev && (directNext ? fMemAllocateIntermediate(prev, next, usage) : fMemExpandPrevious(prev, size, usage))) {
		fFlushCaches();
		return true;
	}

	/* lookup the previous physical page and split it up such that it describes the single contigous block
	*	of previous virtual memory (upon releasing the moved blocks, they will automatically be merged) */
	detail::MemPhysIt physPrev = (directPrev ? fLookupPhysical(prev->second.physical) : pPhysical.end());
	if (physPrev != pPhysical.end() && physPrev->second.users > 1)
		physPrev = pPhysical.end();
	if (physPrev != pPhysical.end()) {
		/* split the physical pages at the upper edge */
		if (fPhysEnd(physPrev) != fPhysEnd(prev))
			fPhysSplit(physPrev, fPhysEnd(prev));

		/* find the first virtual page, which is less/equal to prev, and lies contiguous both in virtual and physical memory */
		detail::MemVirtIt first = prev;
		while (first != pVirtual.begin()) {
			detail::MemVirtIt temp = std::prev(first);

			/* check if the next virtual range lies contigous to the current next range */
			if (fVirtEnd(temp) != first->first)
				break;

			/* check if it also lies contigous in physical memory */
			if (fPhysEnd(temp) != first->second.physical || first->second.physical == physPrev->first)
				break;
			first = temp;
		}

		/* split the physical pages at the lower edge */
		if (first->second.physical != physPrev->first)
			physPrev = fPhysSplit(physPrev, first->second.physical);
	}

	/* lookup the next physical page and split it up such that it describes the single contigous block
	*	of upcoming virtual memory (upon releasing the moved blocks, they will automatically be merged) */
	detail::MemPhysIt physNext = (directNext ? fLookupPhysical(next->second.physical) : pPhysical.end());
	if (physNext != pPhysical.end() && physNext->second.users > 1)
		physNext = pPhysical.end();
	if (physNext != pPhysical.end()) {
		/* split the physical pages at the lower edge */
		if (physNext->first != next->second.physical)
			physNext = fPhysSplit(physNext, next->second.physical);

		/* find the last virtual page, which is greater/equal to next, and lies contiguous both in virtual and physical memory */
		detail::MemVirtIt last = next;
		while (true) {
			detail::MemVirtIt temp = std::next(last);

			/* check if the next virtual range lies contigous to the current next range */
			if (temp == pVirtual.end() || temp->first != fVirtEnd(last))
				break;

			/* check if it also lies contigous in physical memory */
			if (temp->second.physical != fPhysEnd(last) || fPhysEnd(last) == fPhysEnd(physNext))
				break;
			last = temp;
		}

		/* split the physical pages at the upper edge */
		if (fPhysEnd(last) != fPhysEnd(physNext))
			fPhysSplit(physNext, fPhysEnd(last));
	}

	/* check if the size can be serviced (i.e. size does not overflow) */
	uint64_t required = (physPrev != pPhysical.end() ? physPrev->second.size : 0) + (physNext != pPhysical.end() ? physNext->second.size : 0);
	if (required + size < required) {
		logger.error(u8"Size cannot be serviced by mmap");
		return false;
	}

	/* lookup a physical slot large enough to house the contiguous memory region */
	detail::MemPhysIt phys = fMemAllocatePhysical(required + size, size);
	if (phys == pPhysical.end()) {
		logger.debug(u8"Allocation failed");
		return false;
	}

	/* insert the new virtual entry */
	detail::MemVirtIt virt = pVirtual.insert({ address, detail::MemoryVirtual{ 0, size, usage } }).first;

	/* merge the previous and next blocks into the new contiguous physical region and merge the virtual slots */
	virt->second.physical = fMemMergePhysical(virt, phys, size, physPrev, physNext);
	fVirtMergePrev(virt);
	if (directNext)
		fVirtMergePrev(next);

	/* clear the next allocated block of memory and check if the physical memory itself should be reduced */
	detail::MemoryBridge::ClearPhysical(virt->second.physical, size);
	fReducePhysical();

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
	return true;
}

void env::Memory::fCheckXInvalidated(env::guest_t address) {
	if (!pXInvalidated)
		return;

	/* flush the caches - to ensure that the next write is detect again */
	pXInvalidated = false;
	fFlushCaches();
	throw env::ExecuteDirty{ address };
}
void env::Memory::fCacheLookup(env::guest_t address, env::guest_t access, uint32_t size, uint32_t usage, uint32_t cache) {
	/* compute the index into the fast-cache */
	uint64_t pageAddress = (access >> pPageBitShift);
	pageAddress ^= (pageAddress >> detail::MemoryFastCacheBits);
	if (usage == env::Usage::Read)
		pageAddress *= detail::MemoryFastCacheConstRead;
	else if (usage == env::Usage::Write)
		pageAddress *= detail::MemoryFastCacheConstWrite;
	else
		pageAddress *= detail::MemoryFastCacheConstElse;
	uint32_t index = uint32_t(pageAddress) & (detail::MemoryFastCount - 1);

	/* check if the memory-cache is already cached in the fast-caches (usage will be null for empty caches) */
	uint64_t offset = (access - pFastCache[index].address);
	if (pFastCache[index].usage == usage && offset < pFastCache[index].size && size <= pFastCache[index].size - offset) {
		pCaches[cache] = {
			pFastCache[index].address,
			pFastCache[index].physical,
			(pFastCache[index].size - 0),
			(pFastCache[index].size - 1),
			(pFastCache[index].size - 3),
			(pFastCache[index].size - 7)
		};
		return;
	}

	/* perform the actual lookup (use size=0 to indicate fast-lookup) */
	detail::MemoryLookup lookup = {};
	if (size != 0) {
		logger.fmtTrace(u8"CheckLookup [{:#018x}] with size [{}] and usage [{}] from [{:#018x}] - index: [{:02}] | fast: [{:02}]",
			access, size, env::Usage::Print{ usage }, address, cache, index);
		lookup = fCheckLookup(address, access, size, usage);
	}
	else {
		logger.fmtTrace(u8"FastLookup [{:#018x}] with usage [{}] - index: [{:02}] | fast: [{:02}]",
			access, env::Usage::Print{ usage }, cache, index);
		lookup = fFastLookup(access, usage);
	}

	/* write the lookup back to the cache */
	pCaches[cache] = {
		lookup.address,
		uint32_t(lookup.physical),
		uint32_t(lookup.size - 0),
		uint32_t(lookup.size - 1),
		uint32_t(lookup.size - 3),
		uint32_t(lookup.size - 7)
	};

	/* write the cache entry also to the fast-cache */
	pFastCache[index] = { lookup.address, uint32_t(lookup.physical), uint32_t(lookup.size), usage };
}
uint64_t env::Memory::fRead(env::guest_t address, uint64_t size) const {
	return detail::MemoryBridge::Read(address, size);
}
void env::Memory::fWrite(env::guest_t address, uint64_t size, uint64_t value) const {
	detail::MemoryBridge::Write(address, size, value);
}
uint64_t env::Memory::fCode(env::guest_t address, uint64_t size) const {
	return detail::MemoryBridge::Code(address, size);
}

uint64_t env::Memory::totalAllocated() const {
	uint64_t total = 0;
	for (const auto& [address, phys] : pPhysical) {
		if (phys.users > 0)
			total += phys.size;
	}
	return total;
}
uint64_t env::Memory::totalShared() const {
	uint64_t total = 0;
	for (const auto& [address, phys] : pPhysical) {
		if (phys.users > 1)
			total += phys.size;
	}
	return total;
}
uint64_t env::Memory::maxAllocate() const {
	return uint64_t(std::numeric_limits<uint32_t>::max()) + 1;
}
void env::Memory::checkXInvalidated(env::guest_t address) {
	fCheckXInvalidated(address);
}
std::pair<env::guest_t, uint64_t> env::Memory::findNext(env::guest_t address) const {
	/* lookup the entry, which contains the given address */
	detail::MemVirtIt virt = pVirtual.upper_bound(address);
	if (virt != pVirtual.begin())
		--virt;

	/* check if the entry contains the address */
	if (virt == pVirtual.end())
		return { 0, 0 };
	if (virt->first <= address && fVirtEnd(virt) > address)
		return { virt->first, virt->second.size };

	/* return the next entry */
	if (++virt == pVirtual.end())
		return { 0, 0 };
	return { virt->first, virt->second.size };
}
uint32_t env::Memory::getUsage(env::guest_t address) const {
	/* lookup the entry, which contains the given address */
	detail::MemVirtIt virt = pVirtual.upper_bound(address);
	if (virt != pVirtual.begin())
		--virt;

	/* check if the entry contains the address */
	if (virt != pVirtual.end() && virt->first <= address && fVirtEnd(virt) > address)
		return virt->second.usage;
	return 0;
}

env::guest_t env::Memory::alloc(uint64_t size, uint32_t usage) {
	/* check if the allocation can be serviced */
	if (size > detail::EndOfAllocations - detail::StartOfAllocations)
		return 0;

	/* fetch the last address currently allocated */
	env::guest_t last = (pVirtual.empty() ? 0 : fVirtEnd(std::prev(pVirtual.end())));

	/* check if the start of allocations can just be taken */
	env::guest_t address = 0;
	if (last == 0 || last <= detail::StartOfAllocations - detail::SpacingBetweenAllocations)
		address = detail::StartOfAllocations;

	else {
		/* check if the next address is still available, while leaving allocation-space for both the last and next allocation */
		uint64_t required = last + 2 * detail::SpacingBetweenAllocations;
		if (required < detail::EndOfAllocations && detail::EndOfAllocations - required >= size)
			address = last + detail::SpacingBetweenAllocations;

		/* check if no allocation exists in the space, in which case size must be very large, and simply place the allocation into the middle of the block */
		else if (last <= detail::StartOfAllocations)
			address = detail::StartOfAllocations + ((detail::EndOfAllocations - detail::StartOfAllocations) / 2);

		/* lookup the largest spot in the allocations, and place the address in the middle */
		else {
			/* virt must exist, as last is larger than start-of-allocations */
			detail::MemVirtIt virt = pVirtual.upper_bound(detail::StartOfAllocations);

			/* setup the initial starting parameter (depending on if the first entry overlaps into the allocation range) */
			env::guest_t start = 0, end = 0, prev = detail::StartOfAllocations;
			if (virt != pVirtual.begin())
				prev = std::max<env::guest_t>(prev, fVirtEnd(std::prev(virt)));

			/* iterate over the upcoming addresses and look for the largest empty slot */
			for (; virt != pVirtual.end(); ++virt) {
				if (virt->first >= detail::EndOfAllocations)
					break;

				/* check if the current spacing is larger */
				env::guest_t space = virt->first - prev;
				if (space > (end - start)) {
					start = prev;
					end = virt->first;
				}
				prev = fVirtEnd(virt);
			}

			/* check if the slot to the end-of-allocations is larger */
			if (prev < detail::EndOfAllocations) {
				env::guest_t space = detail::EndOfAllocations - prev;
				if (space > (end - start)) {
					start = prev;
					end = detail::EndOfAllocations;
				}
			}

			/* check if a matching slot has been found and compute the address to be used */
			if ((end - start) < size)
				return 0;
			address = start + ((end - start - size) / 2);
		}
	}

	/* align the final address and try to perform the allocation */
	address &= ~(pPageSize - 1);
	if (!fMMap(address, size, usage))
		return 0;
	return address;
}
bool env::Memory::mmap(env::guest_t address, uint64_t size, uint32_t usage) {
	return fMMap(address, size, usage);
}
bool env::Memory::munmap(env::guest_t address, uint64_t size) {
	logger.debug(u8"Unmapping [", str::As{ U"#018x", address }, u8"] with size [", str::As{ U"#010x", size }, u8']');

	/* check if the address and size are aligned properly */
	if (fPageOffset(address) != 0 || fPageOffset(size) != 0) {
		logger.error(u8"Unmapping requires address and size to be page-aligned");
		return false;
	}

	/* check if the size overflows */
	env::guest_t endAddress = address + size;
	if (endAddress < address) {
		logger.error(u8"Size overflows for operation");
		return false;
	}
	if (size == 0)
		return true;

	/* check if the entire given range lies within a mapped region */
	detail::MemVirtIt begin = fLookupVirtual(address);
	detail::MemVirtIt last = begin;
	while (last != pVirtual.end() && fVirtEnd(last) < endAddress) {
		detail::MemVirtIt next = std::next(last);

		/* check if the next entry is mapped and lies contiguously in memory */
		if (next == pVirtual.end() || fVirtEnd(last) != next->first)
			last = pVirtual.end();
		else
			last = next;
	}
	if (last == pVirtual.end()) {
		logger.error(u8"Unmapping range is not fully mapped");
		return false;
	}

	/* break the first and last virtual memory at the boundaries */
	if (address > begin->first) {
		detail::MemVirtIt temp = fVirtSplit(begin, address);
		if (last == begin)
			last = temp;
		begin = temp;
	}
	if (endAddress < fVirtEnd(last))
		fVirtSplit(last, endAddress);

	/* remove all intermediate pages */
	detail::MemPhysIt phys = pPhysical.end();
	detail::MemVirtIt end = std::next(last);
	while (begin != end) {
		uint64_t phAddress = begin->second.physical, phEnd = fPhysEnd(begin);

		/* check if the physical page still matches or lookup the next proper page */
		if (phys == pPhysical.end() || phys->first > phAddress || fPhysEnd(phys) < phAddress)
			phys = fLookupPhysical(phAddress);

		/* release all physical pages, which contain the current virtual page */
		while (phAddress < phEnd) {
			/* break the physical memory at the lower and upper edge */
			if (phys->first < phAddress)
				phys = fPhysSplit(phys, phAddress);
			if (phEnd < fPhysEnd(phys))
				fPhysSplit(phys, phEnd);
			phAddress = fPhysEnd(phys);

			/* unmap the physical memory and try to merge it with the neighboring entries (will not merge with just-split
			*	entries, as this is now considered used one time less, while the other one's are still considered more used) */
			--phys->second.users;
			phys = std::next(fPhysMerge(phys));
		}

		/* check if the virtual memory contained executable memory, which is now being removed - marks executables as invalidated */
		if ((begin->second.usage & env::Usage::Execute) != 0)
			pXInvalidated = true;

		/* remove the virtual page */
		begin = pVirtual.erase(begin);
	}

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
	return true;
}
bool env::Memory::mprotect(env::guest_t address, uint64_t size, uint32_t usage) {
	logger.fmtDebug(u8"Changing [{:#018x}] with size [{:#010x}] and usage [{}]", address, size, env::Usage::Print{ usage });

	/* check if the address and size are aligned properly and validate the usage */
	if (fPageOffset(address) != 0 || fPageOffset(size) != 0) {
		logger.error(u8"Changing requires address and size to be page-aligned");
		return false;
	}
	if ((usage & ~env::Usage::Usages) != 0) {
		logger.error(u8"Memory-usage must only consist of read/write/execute/lock usage");
		return false;
	}

	/* check if the size overflows */
	env::guest_t endAddress = address + size;
	if (endAddress < address) {
		logger.error(u8"Size overflows for operation");
		return false;
	}
	if (size == 0)
		return true;

	/* check if the entire given range lies within a mapped region */
	detail::MemVirtIt begin = fLookupVirtual(address);
	detail::MemVirtIt last = begin;
	while (last != pVirtual.end() && fVirtEnd(last) < endAddress) {
		detail::MemVirtIt next = std::next(last);

		/* check if the next entry is mapped and lies contiguously in memory */
		if (next == pVirtual.end() || fVirtEnd(last) != next->first)
			last = pVirtual.end();
		else
			last = next;
	}
	if (last == pVirtual.end()) {
		logger.error(u8"Change range is not fully mapped");
		return false;
	}

	/* break the first and last virtual memory at the boundaries */
	if (address > begin->first) {
		detail::MemVirtIt temp = fVirtSplit(begin, address);
		if (last == begin)
			last = temp;
		begin = temp;
	}
	if (endAddress < fVirtEnd(last))
		fVirtSplit(last, endAddress);

	/* patch all intermediate pages */
	detail::MemVirtIt end = std::next(last);
	while (begin != end) {
		/* check if the virtual memories x-bit changes which requires executable invalidation */
		if ((begin->second.usage & env::Usage::Execute) != (usage & env::Usage::Execute))
			pXInvalidated = true;

		/* update the usage of the virtual memory and try to merge it with the previous range */
		begin->second.usage = usage;
		begin = std::next(fVirtMergePrev(begin));
	}

	/* merge the final virtual page with the neighbor */
	if (end != pVirtual.end())
		fVirtMergePrev(end);

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
	return true;
}
void env::Memory::mread(void* dest, env::guest_t source, uint64_t size, uint32_t usage) {
	logger.fmtTrace(u8"Reading [{:#018x}] with size [{:#010x}] and usage [{}]", source, size, env::Usage::Print{ usage });
	if (size == 0)
		return;
	uint8_t* ptr = static_cast<uint8_t*>(dest);

	/* lookup the address to ensure it is mapped and perform the read operations */
	detail::MemoryLookup lookup = fCheckLookup(detail::MainAccessAddress, source, size, usage);
	while (true) {
		uint64_t offset = (source - lookup.address);
		uint64_t count = std::min<uint64_t>(lookup.size - offset, size);
		detail::MemoryBridge::ReadFromPhysical(ptr, lookup.physical + offset, count);

		/* check if the end has been reached and otherwise advance the parameter */
		if (count >= size)
			return;
		ptr += count;
		source += count;
		size -= count;
		lookup = fFastLookup(source, usage);
	}
}
void env::Memory::mwrite(env::guest_t dest, const void* source, uint64_t size, uint32_t usage) {
	logger.fmtTrace(u8"Writing [{:#018x}] with size [{:#010x}] and usage [{}]", dest, size, env::Usage::Print{ usage });
	if (size == 0)
		return;
	const uint8_t* ptr = static_cast<const uint8_t*>(source);

	/* lookup the address to ensure it is mapped and perform the write operations */
	detail::MemoryLookup lookup = fCheckLookup(detail::MainAccessAddress, dest, size, usage);
	while (true) {
		uint64_t offset = (dest - lookup.address);
		uint64_t count = std::min<uint64_t>(lookup.size - offset, size);
		detail::MemoryBridge::WriteToPhysical(lookup.physical + offset, ptr, count);

		/* check if the end has been reached and otherwise advance the parameter */
		if (count >= size)
			return;
		ptr += count;
		dest += count;
		size -= count;
		lookup = fFastLookup(dest, usage);
	}
}
void env::Memory::mclear(env::guest_t dest, uint64_t size, uint32_t usage) {
	logger.fmtTrace(u8"Clearing [{:#018x}] with size [{:#010x}] and usage [{}]", dest, size, env::Usage::Print{ usage });
	if (size == 0)
		return;

	/* lookup the address to ensure it is mapped and perform the clear operations */
	detail::MemoryLookup lookup = fCheckLookup(detail::MainAccessAddress, dest, size, usage);
	while (true) {
		uint64_t offset = (dest - lookup.address);
		uint64_t count = std::min<uint64_t>(lookup.size - offset, size);
		detail::MemoryBridge::ClearPhysical(lookup.physical + offset, count);

		/* check if the end has been reached and otherwise advance the parameter */
		if (count >= size)
			return;
		dest += count;
		size -= count;
		lookup = fFastLookup(dest, usage);
	}
}
