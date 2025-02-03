#include "env-memory.h"

static util::Logger logger{ u8"env::memory" };

size_t env::Memory::fLookupVirtual(env::guest_t address) const {
	size_t begin = 0, end = pVirtual.size();
	while ((end - begin) > 1) {
		size_t index = (begin + end + 1) / 2;
		if (address < pVirtual[index].address)
			end = index;
		else
			begin = index;
	}
	return begin;
}
env::detail::MemPhysIt env::Memory::fLookupPhysical(uint64_t physical) const {
	env::detail::MemPhysIt it = pPhysical.upper_bound(physical);

	/* check if no entry is greater than the physical address */
	if (it == pPhysical.begin())
		return pPhysical.end();
	--it;

	/* check if the range contains the given address (cannot be equal to end) */
	if (it->first + it->second.size <= physical)
		return pPhysical.end();
	return it;
}
env::detail::MemoryLookup env::Memory::fLookup(env::guest_t address, env::guest_t access, uint64_t size, uint32_t usage) const {
	/* lookup the virtual mapping containing the corresponding accessed-address */
	size_t index = fLookupVirtual(access);

	/* check if an entry has been found */
	if (index >= pVirtual.size() || access < pVirtual[index].address || access >= pVirtual[index].address + pVirtual[index].size)
		throw env::MemoryFault{ address, access, size, usage, 0 };

	/* check if the usage attributes are valid */
	if ((pVirtual[index].usage & usage) != usage)
		throw env::MemoryFault{ address, access, size, usage, pVirtual[index].usage };

	/* collect all previous and upcoming regions of the same usage */
	detail::MemoryLookup lookup = detail::MemoryLookup{ pVirtual[index].address, pVirtual[index].physical, pVirtual[index].size };
	for (size_t i = index; i > 0; --i) {
		if (pVirtual[i - 1].address + pVirtual[i - 1].size != pVirtual[i].address)
			break;
		if ((pVirtual[i - 1].usage & usage) != usage)
			break;
		lookup = { pVirtual[i - 1].address, pVirtual[i - 1].physical, pVirtual[i - 1].size + lookup.size };
	}
	for (size_t i = index + 1; i < pVirtual.size(); ++i) {
		if (lookup.address + lookup.size != pVirtual[i].address)
			break;
		if ((pVirtual[i].usage & usage) != usage)
			break;
		lookup.size += pVirtual[i].size;
	}

	/* check if size-validation needs to be performed or if the entire range can be serviced by this lookup */
	if (size == 0 || lookup.address + lookup.size >= access + size)
		return lookup;

	/* check if the upcoming address is also mapped and valid
	*	Note: as of now, double-mapping is not yet active, therefore the lookup will always consume the entire range, and
	*	there is no chance of it not being contiguous, therefore only the size of the first lookup needs to be checked */
	throw env::MemoryFault{ address, access, size, usage, 0 };
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
	if (detail::MemoryBridge::ExpandPhysical(pages))
		return pages * detail::PhysPageSize;
	size = detail::PhysPageCount(size);
	if (size < pages && detail::MemoryBridge::ExpandPhysical(size))
		return (size * detail::PhysPageSize);
	return 0;
}
void env::Memory::fMovePhysical(uint64_t dest, uint64_t source, uint64_t size) const {
	detail::MemoryBridge::MovePhysical(dest, source, size);
}
void env::Memory::fFlushCaches() {
	fCheckConsistency();
	logger.trace(u8"Flushing caches");
	std::memset(pCaches.data(), 0, sizeof(detail::MemoryCache) * pCaches.size());
	std::memset(pFastCache, 0, sizeof(detail::MemoryFast) * detail::MemoryFastCount);
}
void env::Memory::fCheckConsistency() const {
	if (pPhysical.empty())
		logger.fatal(u8"Physical slots invalid");

	uint64_t totalPhys = 0, totalVirt = 0, walkPhys = 0;
	bool lastUsed = true;
	for (const auto& [address, phys] : pPhysical) {
		totalPhys += (phys.used ? phys.size : 0);

		if (phys.size == 0 || fPageOffset(phys.size) != 0)
			logger.fatal(u8"Physical slot [", str::As{ "#018x", address }, u8"] size is invalid");

		if (address != walkPhys)
			logger.fatal(u8"Physical slot [", str::As{ "#018x", address }, u8"] address is invalid");
		if (!lastUsed && !phys.used)
			logger.fatal(u8"Physical slot [", str::As{ "#018x", address }, u8"] usage is invalid");

		walkPhys += phys.size;
		lastUsed = phys.used;
	}

	for (size_t i = 0; i < pVirtual.size(); ++i) {
		totalVirt += pVirtual[i].size;

		if (fLookupVirtual(pVirtual[i].address) != i)
			logger.fatal(u8"Virtual slot [", i, u8"] lookup failed");

		detail::MemPhysIt phys = fLookupPhysical(pVirtual[i].physical);
		if (phys == pPhysical.end() || !phys->second.used || pVirtual[i].physical < phys->first ||
			pVirtual[i].physical + pVirtual[i].size > phys->first + phys->second.size)
			logger.fatal(u8"Virtual slot [", i, u8"] physical is invalid");

		if (pVirtual[i].size == 0 || fPageOffset(pVirtual[i].size) != 0)
			logger.fatal(u8"Virtual slot [", i, u8"] size is invalid");
		if (i == 0)
			continue;

		if (pVirtual[i - 1].address + pVirtual[i - 1].size > pVirtual[i].address)
			logger.fatal(u8"Virtual slot [", i, u8"] address is invalid");
		if (pVirtual[i - 1].address + pVirtual[i - 1].size < pVirtual[i].address)
			continue;

		if (pVirtual[i - 1].physical + pVirtual[i - 1].size != pVirtual[i].physical)
			logger.fatal(u8"Virtual slot [", i, u8"] physical is invalid");
		if (pVirtual[i - 1].usage == pVirtual[i].usage)
			logger.fatal(u8"Virtual slot [", i, u8"] usage is invalid");
	}

	if (totalPhys != totalVirt)
		logger.fatal(u8"Phyiscal used memory does not match virtual used memory");
	logger.debug(u8"Memory consistency-check performed");
}
bool env::Memory::fCheckAllMapped(size_t virt, env::guest_t end) const {
	env::guest_t address = pVirtual[virt].address;

	/* check if the entire range is mapped correctly */
	while (virt < pVirtual.size() && pVirtual[virt].address == address) {
		address = pVirtual[virt].address + pVirtual[virt].size;
		++virt;
		if (end <= address)
			return true;
	}
	return false;
}

void env::Memory::fMemMakePhysicalUnused(detail::MemPhysIt phys) {
	phys->second.used = false;

	/* check if the next entry is unused as well */
	detail::MemPhysIt next = std::next(phys);
	if (next != pPhysical.end() && !next->second.used) {
		phys->second.size += next->second.size;
		pPhysical.erase(next);
	}

	/* check if the previous entry is unused as well */
	next = phys;
	if (phys != pPhysical.begin() && !(--phys)->second.used) {
		phys->second.size += next->second.size;
		pPhysical.erase(next);
	}
}
bool env::Memory::fMemExpandPrevious(size_t virt, env::guest_t address, uint64_t size, uint32_t usage) {
	detail::MemPhysIt phys = fLookupPhysical(pVirtual[virt].physical);

	/* check if the next page is already used, in which case no expansion is possible */
	detail::MemPhysIt next = std::next(phys);
	if (next != pPhysical.end() && next->second.used)
		return false;

	/* check if the next physical page can be consumed */
	if (next != pPhysical.end() && next->second.size >= size) {
		phys->second.size += size;

		/* check if a new intermediate block needs to be inserted */
		if (size < next->second.size)
			pPhysical.insert(next, { next->first + size, detail::MemoryPhysical{ next->second.size - size, false } });
		pPhysical.erase(next);
	}

	/* expand the physical memory at the end */
	else {
		/* check if the physical page is at the end */
		if (next != pPhysical.end() && std::next(next) != pPhysical.end())
			return false;

		/* try to expand the physical memory (may allocate more than the needed amount) */
		uint64_t needed = size - (next != pPhysical.end() ? next->second.size : 0);
		uint64_t allocate = fExpandPhysical(needed, size);
		if (allocate == 0)
			return false;

		/* merge the size of the next unused block with the current unused block */
		if (next != pPhysical.end()) {
			allocate += next->second.size;
			pPhysical.erase(next);
		}

		/* patch the size of the expanded block and insert the new block of unused size */
		phys->second.size += size;
		if (allocate > size)
			pPhysical.insert(phys, { phys->first + phys->second.size, detail::MemoryPhysical{ allocate - size, false } });
	}

	/* update the virtual slot */
	uint64_t physical = pVirtual[virt].physical + pVirtual[virt].size;
	if (pVirtual[virt].usage == usage)
		pVirtual[virt].size += size;
	else
		pVirtual.insert(pVirtual.begin() + virt + 1, detail::MemoryVirtual{ address, physical, size, usage });

	/* clear the next allocated block of memory */
	detail::MemoryBridge::ClearPhysical(physical, size);
	return true;
}
env::detail::MemPhysIt env::Memory::fMemAllocatePhysical(uint64_t size, uint64_t growth) {
	detail::MemPhysIt phys = pPhysical.begin();

	/* look for the next region large enough to store the required memory */
	for (; phys != pPhysical.end(); ++phys) {
		if (!phys->second.used && phys->second.size >= size)
			break;
	}

	/* check if the physical memory needs to be allocated */
	if (phys == pPhysical.end()) {
		uint64_t allocate = fExpandPhysical(size, growth);
		if (allocate == 0)
			return phys;

		/* add the newly allocated physical memory/slot (physical map can never be empty,
		*	as it will always be initialized with at least some unused memory) */
		--phys;
		if (!phys->second.used)
			phys->second.size += allocate;
		else
			pPhysical.insert(phys, { phys->first + phys->second.size,  detail::MemoryPhysical{ allocate, false } });
	}
	return phys;
}
bool env::Memory::fMemAllocateIntermediate(size_t virt, uint64_t size, uint32_t usage) {
	/* check if the previous and next block already lie aligned in physical memory */
	if (pVirtual[virt].physical + pVirtual[virt].size + size != pVirtual[virt + 1].physical)
		return false;

	/* check if the intermediate physical block is not yet allocated (next block
	*	must exist, as the upcoming virtual entry is at a higher address) */
	detail::MemPhysIt phys = fLookupPhysical(pVirtual[virt].physical);
	detail::MemPhysIt next = std::next(phys);
	if (next->second.used || next->second.size != size)
		return false;

	/* clear the next allocated block of memory */
	detail::MemoryBridge::ClearPhysical(next->first, next->second.size);

	/* merge the three physical ranges back together */
	detail::MemPhysIt last = std::next(next);
	phys->second.size += next->second.size + last->second.size;
	pPhysical.erase(next);
	pPhysical.erase(last);

	/* check if the lower virtual block can consume the current block */
	if (pVirtual[virt].usage == usage) {
		pVirtual[virt].size += size;

		/* check if it can also consume the upper block */
		if (pVirtual[virt + 1].usage == usage) {
			pVirtual[virt].size += pVirtual[virt + 1].size;
			pVirtual.erase(pVirtual.begin() + virt + 1);
		}
		return true;
	}

	/* check if the upper virtual block can consume the current block */
	if (pVirtual[virt + 1].usage == usage) {
		pVirtual[virt + 1].address -= size;
		pVirtual[virt + 1].physical -= size;
		pVirtual[virt + 1].size += size;
		return true;
	}

	/* insert the new virtual block */
	pVirtual.insert(pVirtual.begin() + virt + 1, detail::MemoryVirtual{ pVirtual[virt].address + pVirtual[virt].size, pVirtual[virt].physical + pVirtual[virt].size, size, usage });
	return true;
}
uint64_t env::Memory::fMemMergePhysical(size_t virt, detail::MemPhysIt phys, uint64_t size, detail::MemPhysIt physPrev, detail::MemPhysIt physNext) {
	uint64_t capacity = phys->second.size;

	/* mark the new physical slot as used and initialize its size */
	phys->second.size = size;
	phys->second.used = true;

	/* move the neighboring regions into place */
	uint64_t physical = phys->first;
	if (physPrev != pPhysical.end()) {
		/* move the memory into place and patch the new physical slot */
		fMovePhysical(physical, physPrev->first, physPrev->second.size);
		phys->second.size += physPrev->second.size;

		/* patch the virtual slots */
		uint64_t offset = physPrev->second.size;
		for (size_t i = virt; offset > 0; --i) {
			offset -= pVirtual[i].size;
			pVirtual[i].physical = physical + offset;
		}
		physical += physPrev->second.size;

		/* mark the previous physical slot as unused */
		fMemMakePhysicalUnused(physPrev);
	}
	if (physNext != pPhysical.end()) {
		/* move the memory into place and patch the old and new physical slot */
		fMovePhysical(physical + size, physNext->first, physNext->second.size);
		phys->second.size += physNext->second.size;

		/* patch the virtual slots */
		uint64_t offset = 0;
		for (size_t i = virt + 1; offset < physNext->second.size; ++i) {
			pVirtual[i].physical = physical + size + offset;
			offset += pVirtual[i].size;
		}

		/* mark the next physical slot as unused */
		fMemMakePhysicalUnused(physNext);
	}

	/* check if capacity remains, which needs to be written to a new slot */
	capacity -= phys->second.size;
	if (capacity > 0) {
		uint64_t end = phys->first + phys->second.size;

		/* check if an existing slot can be expanded */
		detail::MemPhysIt next = std::next(phys);
		if (next != pPhysical.end() && !next->second.used)
			next->second.size += capacity;

		/* allocate or insert the new physical slot */
		else
			pPhysical.insert(next, { end, detail::MemoryPhysical{ capacity, false } });
	}
	return physical;
}
bool env::Memory::fMMap(env::guest_t address, uint64_t size, uint32_t usage) {
	logger.fmtDebug(u8"Mapping [{:#018x}] with size [{:#010x}] and usage [{}]", address, size, env::Usage::Print{ usage });

	/* check if the address and size are aligned properly and validate the usage */
	if (fPageOffset(address) != 0 || fPageOffset(size) != 0 || size == 0) {
		logger.error(u8"Mapping requires address and size to be page-aligned and size greater than zero");
		return false;
	}
	if ((usage & ~env::Usage::Usages) != 0) {
		logger.error(u8"Memory-usage must only consist of read/write/execute usage");
		return false;
	}

	/* check if the given range is valid and does not overlap any existing ranges */
	size_t virt = fLookupVirtual(address);
	if (virt < pVirtual.size() && address >= pVirtual[virt].address && address < pVirtual[virt].address + pVirtual[virt].size) {
		logger.error(u8"Mapping range is already partially mapped");
		return false;
	}
	if (virt + 1 < pVirtual.size() && pVirtual[virt + 1].address - address < size) {
		logger.error(u8"Mapping range is already partially mapped");
		return false;
	}

	/* check if the previous or neighboring virtual page is already allocated */
	bool hasPrev = (!pVirtual.empty() && pVirtual[virt].address + pVirtual[virt].size == address);
	bool hasNext = (virt + 1 < pVirtual.size() && address + size == pVirtual[virt + 1].address);

	/* check if an existing neighboring region can just be expanded */
	if (hasPrev && !hasNext && fMemExpandPrevious(virt, address, size, usage)) {
		fFlushCaches();
		return true;
	}

	/* edge-case of previous and next already being properly mapped */
	if (hasPrev && hasNext && fMemAllocateIntermediate(virt, size, usage)) {
		/* flush the caches to ensure the new mapping is applied */
		fFlushCaches();
		return true;
	}

	/* lookup the previous and next physical page */
	detail::MemPhysIt physPrev = (hasPrev ? fLookupPhysical(pVirtual[virt].physical) : pPhysical.end());
	detail::MemPhysIt physNext = (hasNext ? fLookupPhysical(pVirtual[virt + 1].physical) : pPhysical.end());

	/* check if the size can be serviced (i.e. size does not overflow) */
	uint64_t required = (hasPrev ? physPrev->second.size : 0) + (hasNext ? physNext->second.size : 0);
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

	/* merge the previous and next blocks into the new contiguous physical region (might invalidate the phys-indices) */
	uint64_t actual = fMemMergePhysical(virt, phys, size, physPrev, physNext);

	/* clear the next allocated block of memory */
	detail::MemoryBridge::ClearPhysical(actual, size);

	/* check if the virtual entry can be merged with the next entry */
	size_t applied = 0;
	if (hasNext && pVirtual[virt + 1].usage == usage) {
		++applied;
		size += pVirtual[virt + 1].size;
		pVirtual[virt + 1] = detail::MemoryVirtual{ address, actual, size, usage };
	}

	/* check if it can be merged into the previous slot */
	if (hasPrev && pVirtual[virt].usage == usage) {
		++applied;
		pVirtual[virt].size += size;
	}

	/* check if a new slot needs to be allocate or even a slot needs to be removed */
	if (pVirtual.empty())
		pVirtual.push_back({ address, actual, size, usage });
	else if (applied == 0)
		pVirtual.insert(pVirtual.begin() + virt + (address >= pVirtual[virt].address ? 1 : 0), { address, actual, size, usage });
	else if (applied == 2)
		pVirtual.erase(pVirtual.begin() + virt + 1);

	/* check if the lowest physical memory should be reduced and moved down
	*	(there must at least be two slots, as an allocation has occurred in this call) */
	uint64_t end = std::prev(pPhysical.end())->first + std::prev(pPhysical.end())->second.size;
	if (!pPhysical.begin()->second.used && pPhysical.begin()->second.size >= (end / detail::ShiftMemoryFactor)) {
		uint64_t shift = pPhysical.begin()->second.size;
		fMovePhysical(0, shift, end - shift);

		/* shift the virtual slots down */
		for (size_t i = 0; i < pVirtual.size(); ++i)
			pVirtual[i].physical -= shift;

		/* move the physical and virtual memory down */
		detail::MemPhysIt it = pPhysical.begin();
		while (true) {
			detail::MemPhysIt nt = std::next(it);

			/* check if the last block has been reached and remove it */
			if (nt == pPhysical.end()) {
				pPhysical.erase(it);
				break;
			}

			/* move the slot down */
			it->second = nt->second;
			it = nt;
		}
	}

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
	return true;
}

void env::Memory::fMemUnmapSingleBlock(size_t virt, env::guest_t address, uint64_t size) {
	const detail::MemoryVirtual entry = pVirtual[virt];

	/* check if the regions align perfectly */
	if (entry.address == address && entry.size == size) {
		pVirtual.erase(pVirtual.begin() + virt);
		return;
	}

	/* check if they align at the lower bound, in which case the slot can just be moved up */
	if (entry.address == address) {
		pVirtual[virt].address += size;
		pVirtual[virt].physical += size;
		pVirtual[virt].size -= size;
		return;
	}

	/* check if they align at the upper bound, in which case the slot can just be reduced in size */
	if (entry.address + entry.size == address + size) {
		pVirtual[virt].size -= size;
		return;
	}

	/* a new slot needs to be inserted above */
	uint64_t offset = address - entry.address;
	pVirtual[virt].size = offset;
	offset += size;
	pVirtual.insert(pVirtual.begin() + virt + 1, detail::MemoryVirtual{ address + size, entry.physical + offset, entry.size - offset, entry.usage });
}
bool env::Memory::fMemUnmapMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end) {
	/* check if the entire range is mapped correctly */
	if (!fCheckAllMapped(virt, end)) {
		logger.error(u8"Unmapping range is not fully mapped");
		return false;
	}

	/* check if the first slot needs to be split up (cannot be double-split, as it must reach across multiple blocks) */
	if (pVirtual[virt].address < address) {
		pVirtual[virt].size = address - pVirtual[virt].address;
		++virt;
	}

	/* remove all remaining blocks until the end has been reached (must all
	*	be valid, as the range has been validated to be fully mapped) */
	size_t dropped = 0;
	while (true) {
		size_t next = virt + dropped;

		/* check if the slot is fully unmapped */
		if (pVirtual[next].address + pVirtual[next].size <= end) {
			++dropped;

			/* check if this was the last block */
			if (pVirtual[next].address + pVirtual[next].size == end)
				break;
			continue;
		}

		/* patch the last block to only contain the remainder */
		uint64_t offset = end - pVirtual[next].address;
		pVirtual[next].address += offset;
		pVirtual[next].physical += offset;
		pVirtual[next].size -= offset;
		break;
	}

	/* drop all cleared blocks */
	if (dropped > 0)
		pVirtual.erase(pVirtual.begin() + virt, pVirtual.begin() + virt + dropped);
	return true;
}
void env::Memory::fMemUnmapPhysical(detail::MemPhysIt phys, uint64_t offset, uint64_t size) {
	/* check if the entire physical range is being removed */
	if (offset == 0 && phys->second.size == size) {
		fMemMakePhysicalUnused(phys);
		return;
	}

	/* check if the physical range aligns with the upper bound */
	if (offset + size == phys->second.size) {
		phys->second.size -= size;

		/* check if it can be merged with the next slot */
		detail::MemPhysIt next = std::next(phys);
		if (next != pPhysical.end() && !next->second.used) {
			pPhysical.insert(next, { next->first - size, detail::MemoryPhysical{ next->second.size + size, false } });
			pPhysical.erase(next);
			return;
		}

		/* insert the new intermediate block */
		pPhysical.insert(phys, { phys->first + phys->second.size, detail::MemoryPhysical{ size, false } });
		return;
	}

	/* check if the physical range aligns with the lower bound */
	if (offset == 0) {
		/* insert the new active slot */
		pPhysical.insert(phys, { phys->first + size, detail::MemoryPhysical{ phys->second.size - size, true } });

		/* check if it can be merged with the previous slot */
		detail::MemPhysIt prev = phys;
		if (prev != pPhysical.begin() && !(--prev)->second.used) {
			prev->second.size += size;
			pPhysical.erase(phys);
			return;
		}

		/* patch the current entry to be unused */
		phys->second.size = size;
		phys->second.used = false;
		return;
	}

	/* insert the two new blocks and populate them and patch the current entry */
	pPhysical.insert(phys, { phys->first + offset + size, detail::MemoryPhysical{ phys->second.size - (offset + size), true } });
	pPhysical.insert(phys, { phys->first + offset, detail::MemoryPhysical{ size, false } });
	phys->second.size = offset;
}

void env::Memory::fMemProtectSingleBlock(size_t virt, env::guest_t address, uint64_t size, uint32_t usage) {
	const detail::MemoryVirtual entry = pVirtual[virt];

	/* check if nothing needs to be done */
	if (entry.usage == usage)
		return;

	/* check if a previous or next block exists */
	bool hasPrev = (virt > 0 && pVirtual[virt - 1].address + pVirtual[virt - 1].size == entry.address);
	bool hasNext = (virt + 1 < pVirtual.size() && entry.address + entry.size == pVirtual[virt + 1].address);

	/* check if the regions align perfectly */
	if (entry.address == address && entry.size == size) {
		pVirtual[virt].usage = usage;

		/* check if it can be merged with the next or previous slot */
		size_t remove = 0;
		if (hasNext && pVirtual[virt + 1].usage == usage) {
			pVirtual[virt].size += pVirtual[virt + 1].size;
			++remove;
		}
		if (hasPrev && pVirtual[virt - 1].usage == usage) {
			--virt;
			pVirtual[virt].size += pVirtual[virt + 1].size;
			++remove;
		}
		if (remove > 0)
			pVirtual.erase(pVirtual.begin() + virt + 1, pVirtual.begin() + virt + 1 + remove);
		return;
	}

	/* check if they align at the lower bound, in which case only one new entry needs to be added */
	if (entry.address == address) {
		/* check if it can be merged with the previous block */
		if (hasPrev && pVirtual[virt - 1].usage == usage) {
			pVirtual[virt - 1].size += size;
			pVirtual[virt].address += size;
			pVirtual[virt].physical += size;
			pVirtual[virt].size -= size;
		}
		else {
			pVirtual.insert(pVirtual.begin() + virt + 1, detail::MemoryVirtual{ entry.address + size, entry.physical + size, entry.size - size, entry.usage });
			pVirtual[virt].size = size;
			pVirtual[virt].usage = usage;
		}
		return;
	}

	/* check if they align at the upper bound, in which case only one new entry needs to be inserted */
	if (entry.address + entry.size == address + size) {
		/* check if it can be merged with the next block */
		if (hasNext && pVirtual[virt + 1].usage == usage) {
			pVirtual[virt + 1].address -= size;
			pVirtual[virt + 1].physical -= size;
			pVirtual[virt + 1].size += size;
			pVirtual[virt].size -= size;
		}
		else {
			pVirtual.insert(pVirtual.begin() + virt + 1, detail::MemoryVirtual{ entry.address + size, entry.physical + size, entry.size - size, usage });
			pVirtual[virt].size = size;
		}
		return;
	}

	/* a new slot needs to be inserted below and above (no possibility to merge with neighboring blocks) */
	pVirtual.insert(pVirtual.begin() + virt + 1, 2, detail::MemoryVirtual{});
	uint64_t offset = address - entry.address;
	pVirtual[virt].size = offset;
	pVirtual[virt + 1] = detail::MemoryVirtual{ address, entry.physical + offset, size, usage };
	pVirtual[virt + 2] = detail::MemoryVirtual{ address + size, entry.physical + offset + size, entry.size - offset - size, entry.usage };
}
bool env::Memory::fMemProtectMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end, uint64_t size, uint32_t usage) {
	bool hasValue = false;

	/* check if the entire range is mapped correctly */
	if (!fCheckAllMapped(virt, end)) {
		logger.error(u8"Change range is not fully mapped");
		return false;
	}

	/* check if the first slot needs to be split up or if it can be used directly */
	uint64_t physical = pVirtual[virt].physical;
	if (pVirtual[virt].address < address) {
		if (pVirtual[virt].usage != usage)
			pVirtual[virt].size = address - pVirtual[virt].address;
		else {
			address = pVirtual[virt].address + pVirtual[virt].size;
			hasValue = true;
		}
		physical += pVirtual[virt].size;
		++virt;
	}

	/* check if the previous block already matches the type */
	else if (virt > 0 && pVirtual[virt - 1].address + pVirtual[virt - 1].size == address && pVirtual[virt - 1].usage == usage)
		hasValue = true;

	/* convert all remaining blocks until the end has been reached (must all
	*	be valid, as the range has been validated to be fully mapped) */
	size_t dropped = 0;
	while (true) {
		size_t next = virt + dropped;

		/* check if the slot is fully mapped across */
		if (pVirtual[next].address + pVirtual[next].size <= end) {
			/* check if the slot can just be added to the previous slot */
			if (hasValue) {
				pVirtual[virt - 1].size += pVirtual[next].size;
				++dropped;
			}

			/* make this the initial slot (cannot be the first slot, as this call requires multiple blocks) */
			else {
				pVirtual[next] = detail::MemoryVirtual{ address, physical, pVirtual[next].size + (pVirtual[next].address - address), usage };
				hasValue = true;
				++virt;
			}

			/* check if this was the last block */
			if (pVirtual[next].address + pVirtual[next].size == end)
				break;
			continue;
		}

		/* check if the last slot is of the same usage and can just be added */
		if (pVirtual[next].usage == usage) {
			if (hasValue) {
				pVirtual[virt - 1].size += pVirtual[next].size;
				++dropped;
			}
			else
				pVirtual[next] = { address, physical, pVirtual[next].size + (pVirtual[next].address - address), usage };
		}

		/* shift the remainder to the next block and either add the rest to the previous block, or insert the new block */
		else {
			uint64_t offset = end - pVirtual[next].address;
			pVirtual[next].address += offset;
			pVirtual[next].physical += offset;
			pVirtual[next].size -= offset;

			/* add the capacity to the previous block or insert the new block */
			if (hasValue)
				pVirtual[virt - 1].size += offset;
			else
				pVirtual.insert(pVirtual.begin() + next, detail::MemoryVirtual{ end, physical, (end - address), usage });
		}
		break;
	}

	/* drop all cleared blocks */
	if (dropped > 0)
		pVirtual.erase(pVirtual.begin() + virt, pVirtual.begin() + virt + dropped);
	return true;
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
			(pFastCache[index].size - 2),
			(pFastCache[index].size - 4),
			(pFastCache[index].size - 8)
		};
		return;
	}

	/* perform the actual lookup */
	logger.fmtTrace(u8"Lookup [{:#018x}] with size [{}] and usage [{}] from [{:#018x}] - index: [{}] | fast: [{}]",
		access, size, env::Usage::Print{ usage }, address, cache, index);
	detail::MemoryLookup lookup = fLookup(address, access, size, usage);

	/* write the lookup back to the cache */
	pCaches[cache] = {
		lookup.address,
		uint32_t(lookup.physical),
		uint32_t(lookup.size - 0),
		uint32_t(lookup.size - 1),
		uint32_t(lookup.size - 2),
		uint32_t(lookup.size - 4),
		uint32_t(lookup.size - 8)
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

env::guest_t env::Memory::alloc(uint64_t size, uint32_t usage) {
	/* check if the allocation can be serviced */
	if (size > detail::EndOfAllocations - detail::StartOfAllocations)
		return 0;

	/* fetch the last address currently allocated */
	env::guest_t last = (pVirtual.empty() ? 0 : pVirtual.back().address + pVirtual.back().size);
	last = (last + pPageSize - 1) & ~(pPageSize - 1);

	/* check if the start of allocations can just be taken */
	env::guest_t address = 0;
	if (last == 0 || last <= detail::StartOfAllocations - detail::SpacingBetweenAllocations)
		address = detail::StartOfAllocations;

	/* check if the next address is still available, while leaving allocation-space for both the last and next allocation */
	else {
		uint64_t required = last + 2 * detail::SpacingBetweenAllocations;
		if (required < detail::EndOfAllocations && detail::EndOfAllocations - required >= size)
			address = last + detail::SpacingBetweenAllocations;

		/* lookup the largest spot in the allocations, and place the address in the middle */
		else {
			/* index must exist, as virtuals cannot be empty, as last would otherwise have been 0) */
			size_t index = fLookupVirtual(detail::StartOfAllocations);

			/* compute the slot to the start-of-allocations address */
			env::guest_t start = 0, end = 0;
			if (pVirtual[index].address > detail::StartOfAllocations) {
				start = detail::StartOfAllocations;
				end = pVirtual[index].address;
			}
			env::guest_t prev = std::max<uint64_t>(detail::StartOfAllocations, pVirtual[index].address + pVirtual[index].size);

			/* iterate over the upcoming addresses and look for the largest empty slot */
			for (++index; index < pVirtual.size(); ++index) {
				if (pVirtual[index].address >= detail::EndOfAllocations)
					break;

				/* check if the current spacing is larger */
				env::guest_t space = pVirtual[index].address - prev;
				if (space > (end - start)) {
					start = prev;
					end = pVirtual[index].address;
				}
				prev = pVirtual[index].address + pVirtual[index].size;
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
			address = start + (((end - start - size) / 2) & ~(pPageSize - 1));
		}
	}

	/* try to perform the allocation */
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

	/* check if the given start is valid and lies within a mapped region */
	size_t virt = fLookupVirtual(address);
	if (virt >= pVirtual.size() || address < pVirtual[virt].address) {
		logger.error(u8"Unmapping range is not fully mapped");
		return false;
	}
	if (size == 0)
		return true;

	/* cache the physical state to be able to patch it afterwards */
	detail::MemPhysIt phys = fLookupPhysical(pVirtual[virt].physical);
	uint64_t offset = (pVirtual[virt].physical - phys->first) + (address - pVirtual[virt].address);

	/* check if the size overflows */
	env::guest_t end = address + size;
	if (end < address) {
		logger.error(u8"Size overflows for operation");
		return false;
	}

	/* check if the range lies on a single block or across multiple slots */
	if (pVirtual[virt].address + pVirtual[virt].size >= end)
		fMemUnmapSingleBlock(virt, address, size);
	else if (!fMemUnmapMultipleBlocks(virt, address, end))
		return false;

	/* unmap the physical memory and flush the caches to ensure the new mapping is accepted */
	fMemUnmapPhysical(phys, offset, size);
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
		logger.error(u8"Memory-usage must only consist of read/write/execute usage");
		return false;
	}

	/* check if the given start is valid and lies within a mapped region */
	size_t virt = fLookupVirtual(address);
	if (virt >= pVirtual.size() || address < pVirtual[virt].address) {
		logger.error(u8"Change range is not fully mapped");
		return false;
	}
	if (size == 0)
		return true;

	/* check if the size overflows */
	env::guest_t end = address + size;
	if (end < address) {
		logger.error(u8"Size overflows for operation");
		return false;
	}

	/* check if the range lies on a single block or across multiple slots */
	if (pVirtual[virt].address + pVirtual[virt].size >= end) {
		if (pVirtual[virt].usage == usage)
			return true;
		fMemProtectSingleBlock(virt, address, size, usage);
	}
	else if (!fMemProtectMultipleBlocks(virt, address, end, size, usage))
		return false;

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
	return true;
}
void env::Memory::mread(void* dest, env::guest_t source, uint64_t size, uint32_t usage) const {
	logger.fmtTrace(u8"Reading [{:#018x}] with size [{:#010x}] and usage [{}]", source, size, env::Usage::Print{ usage });
	if (size == 0)
		return;
	uint8_t* ptr = static_cast<uint8_t*>(dest);

	/* lookup the address to ensure it is mapped and perform the read operations */
	detail::MemoryLookup lookup = fLookup(detail::MainAccessAddress, source, size, usage);
	while (true) {
		uint64_t offset = (source - lookup.address);
		uint64_t count = std::min<uint64_t>(lookup.size - offset, size);
		detail::MemoryBridge::ReadFromPhysical(ptr, lookup.physical + offset, count);

		/* check if the end has been reached and otherwise advance the parameter (no need to perform size
		*	validations again, as the first lookup will have ensured that the entire range is mapped correctly) */
		if (count >= size)
			return;
		ptr += count;
		source += count;
		size -= count;
		lookup = fLookup(detail::MainAccessAddress, source, 0, usage);
	}
}
void env::Memory::mwrite(env::guest_t dest, const void* source, uint64_t size, uint32_t usage) {
	logger.fmtTrace(u8"Writing [{:#018x}] with size [{:#010x}] and usage [{}]", dest, size, env::Usage::Print{ usage });
	if (size == 0)
		return;
	const uint8_t* ptr = static_cast<const uint8_t*>(source);

	/* lookup the address to ensure it is mapped and perform the write operations */
	detail::MemoryLookup lookup = fLookup(detail::MainAccessAddress, dest, size, usage);
	while (true) {
		uint64_t offset = (dest - lookup.address);
		uint64_t count = std::min<uint64_t>(lookup.size - offset, size);
		detail::MemoryBridge::WriteToPhysical(lookup.physical + offset, ptr, count);

		/* check if the end has been reached and otherwise advance the parameter (no need to perform size
		*	validations again, as the first lookup will have ensured that the entire range is mapped correctly) */
		if (count >= size)
			return;
		ptr += count;
		dest += count;
		size -= count;
		lookup = fLookup(detail::MainAccessAddress, dest, 0, usage);
	}
}
void env::Memory::mclear(env::guest_t dest, uint64_t size, uint32_t usage) {
	logger.fmtTrace(u8"Clearing [{:#018x}] with size [{:#010x}] and usage [{}]", dest, size, env::Usage::Print{ usage });
	if (size == 0)
		return;

	/* lookup the address to ensure it is mapped and perform the clear operations */
	detail::MemoryLookup lookup = fLookup(detail::MainAccessAddress, dest, size, usage);
	while (true) {
		uint64_t offset = (dest - lookup.address);
		uint64_t count = std::min<uint64_t>(lookup.size - offset, size);
		detail::MemoryBridge::ClearPhysical(lookup.physical + offset, count);

		/* check if the end has been reached and otherwise advance the parameter (no need to perform size
		*	validations again, as the first lookup will have ensured that the entire range is mapped correctly) */
		if (count >= size)
			return;
		dest += count;
		size -= count;
		lookup = fLookup(detail::MainAccessAddress, dest, 0, usage);
	}
}
std::pair<env::guest_t, uint64_t> env::Memory::findNext(env::guest_t address) const {
	size_t index = fLookupVirtual(address);

	/* check if the index lies to the right of all mappings */
	if (index >= pVirtual.size() || address >= pVirtual[index].address + pVirtual[index].size)
		return { 0, 0 };

	/* return the mapping, which contains this address, or the upcoming mapping */
	return { pVirtual[index].address, pVirtual[index].size };
}
