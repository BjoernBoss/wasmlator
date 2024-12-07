#include "env-memory.h"

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
size_t env::Memory::fLookupPhysical(env::detail::physical_t physical) const {
	size_t begin = 0, end = pPhysical.size();
	while ((end - begin) > 1) {
		size_t index = (begin + end + 1) / 2;
		if (physical < pPhysical[index].physical)
			end = index;
		else
			begin = index;
	}
	return begin;
}
env::detail::MemoryLookup env::Memory::fLookup(env::guest_t address, env::guest_t access, uint32_t size, uint32_t usage) const {
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
		if (pVirtual[i - 1].address + uint64_t(pVirtual[i - 1].size) != pVirtual[i].address)
			break;
		if ((pVirtual[i - 1].usage & usage) != usage)
			break;
		lookup = { pVirtual[i - 1].address, pVirtual[i - 1].physical, pVirtual[index].size + lookup.size };
	}
	for (size_t i = index + 1; i < pVirtual.size(); ++i) {
		if (lookup.address + lookup.size != pVirtual[i].address)
			break;
		if ((pVirtual[i].usage & usage) != usage)
			break;
		lookup.size += pVirtual[i].size;
	}

	/* check if the access-size is valid */
	if (lookup.address + lookup.size < access + size)
		throw env::MemoryFault{ address, access, size, usage, 0 };
	return lookup;
}

uint32_t env::Memory::fPageOffset(env::guest_t address) const {
	return uint32_t(address & uint64_t(pPageSize - 1));
}
uint32_t env::Memory::fExpandPhysical(uint32_t size, uint32_t growth) const {
	/* allocate a little bit more to reduce the number of growings */
	uint32_t pages = detail::PhysPageCount(std::max<uint32_t>(detail::MinGrowthPages * pPageSize, size + growth));
	if (detail::MemoryBridge::ExpandPhysical(pages))
		return uint32_t(pages * detail::PhysPageSize);
	size = detail::PhysPageCount(size);
	if (size < pages && detail::MemoryBridge::ExpandPhysical(size))
		return uint32_t(size * detail::PhysPageSize);
	return 0;
}
void env::Memory::fMovePhysical(env::detail::physical_t dest, env::detail::physical_t source, uint32_t size) const {
	detail::MemoryBridge::MovePhysical(dest, source, size);
}
void env::Memory::fFlushCaches() const {
	fCheckConsistency();

	host::Debug(u8"Flushing caches");
	std::memset(pCaches.data(), 0, sizeof(detail::MemoryCache) * pCaches.size());
}
void env::Memory::fCheckConsistency() const {
	if (pPhysical.empty())
		host::Fatal(u8"Physical slots invalid");

	uint32_t totalPhys = 0, totalVirt = 0;
	for (size_t i = 0; i < pPhysical.size(); ++i) {
		totalPhys += (pPhysical[i].used ? pPhysical[i].size : 0);

		if (fLookupPhysical(pPhysical[i].physical) != i)
			host::Fatal(u8"Physical slot [", i, u8"] lookup failed");
		if (pPhysical[i].size == 0 || fPageOffset(pPhysical[i].size) != 0)
			host::Fatal(u8"Physical slot [", i, u8"] size is invalid");

		if ((i > 0 ? (pPhysical[i - 1].physical + pPhysical[i - 1].size) : 0) != pPhysical[i].physical)
			host::Fatal(u8"Physical slot [", i, u8"] address is invalid");
		if (i > 0 && !pPhysical[i - 1].used && !pPhysical[i].used)
			host::Fatal(u8"Physical slot [", i, u8"] usage is invalid");
	}

	for (size_t i = 0; i < pVirtual.size(); ++i) {
		totalVirt += pVirtual[i].size;

		if (fLookupVirtual(pVirtual[i].address) != i)
			host::Fatal(u8"Virtual slot [", i, u8"] lookup failed");

		size_t phys = fLookupPhysical(pVirtual[i].physical);
		if (phys >= pPhysical.size() || !pPhysical[phys].used || pVirtual[i].physical < pPhysical[phys].physical ||
			pVirtual[i].physical + pVirtual[i].size > pPhysical[phys].physical + pPhysical[phys].size)
			host::Fatal(u8"Virtual slot [", i, u8"] physical is invalid");

		if (pVirtual[i].size == 0 || fPageOffset(pVirtual[i].size) != 0)
			host::Fatal(u8"Virtual slot [", i, u8"] size is invalid");
		if (i == 0)
			continue;

		if (pVirtual[i - 1].address + uint64_t(pVirtual[i - 1].size) > pVirtual[i].address)
			host::Fatal(u8"Virtual slot [", i, u8"] address is invalid");
		if (pVirtual[i - 1].address + uint64_t(pVirtual[i - 1].size) < pVirtual[i].address)
			continue;

		if (pVirtual[i - 1].physical + pVirtual[i - 1].size != pVirtual[i].physical)
			host::Fatal(u8"Virtual slot [", i, u8"] physical is invalid");
		if (pVirtual[i - 1].usage == pVirtual[i].usage)
			host::Fatal(u8"Virtual slot [", i, u8"] usage is invalid");
	}

	if (totalPhys != totalVirt)
		host::Fatal(u8"Phyiscal used memory does not match virtual used memory");
	host::Debug(u8"Memory consistency-check performed");
}

bool env::Memory::fMemExpandPrevious(size_t virt, env::guest_t address, uint32_t size, uint32_t usage) {
	size_t phys = fLookupPhysical(pVirtual[virt].physical);

	/* check if the next page is already used, in which case no expansion is possible */
	if (phys + 1 < pPhysical.size() && pPhysical[phys + 1].used)
		return false;

	/* check if the next physical page can be consumed */
	if (phys + 1 < pPhysical.size() && pPhysical[phys + 1].size >= size) {
		pPhysical[phys].size += size;

		/* either reduce the next physical slot or remove it entirely if consumed */
		if (size < pPhysical[phys + 1].size) {
			pPhysical[phys + 1].physical += size;
			pPhysical[phys + 1].size -= size;
		}
		else
			pPhysical.erase(pPhysical.begin() + phys + 1);
	}

	/* expand the physical memory at the end */
	else {
		/* try to expand the physical memory (may allocate more than the needed amount) */
		uint32_t needed = size - (phys + 1 < pPhysical.size() ? pPhysical[phys + 1].size : 0);
		uint32_t allocate = fExpandPhysical(needed, size);
		if (allocate == 0)
			return false;

		/* check if the entire memory has been consumed */
		if (allocate == needed) {
			if (phys + 1 >= pPhysical.size())
				pPhysical.pop_back();
			pPhysical[phys].size += size;
		}

		/* add the empty block at the end to house the newly allocated overhead */
		else {
			if (phys + 1 >= pPhysical.size())
				pPhysical.push_back(detail::MemoryPhysical{ 0, allocate, false });
			else
				pPhysical[phys + 1].size += allocate;

			pPhysical[phys].size += size;
			pPhysical[phys + 1].physical = pPhysical[phys].physical + pPhysical[phys].size;
			pPhysical[phys + 1].size -= size;
		}
	}

	/* update the virtual slot and the current physical page slot */
	if (pVirtual[virt].usage == usage)
		pVirtual[virt].size += size;
	else {
		detail::physical_t actual = pVirtual[virt].physical + pVirtual[virt].size;
		pVirtual.insert(pVirtual.begin() + virt + 1, detail::MemoryVirtual{ address, actual, size, usage });
	}
	return true;
}
size_t env::Memory::fMemAllocatePhysical(uint32_t size, uint32_t growth) {
	size_t phys = 0;

	/* look for the next region large enough to store the required memory */
	for (; phys < pPhysical.size(); ++phys) {
		if (!pPhysical[phys].used && pPhysical[phys].size >= size)
			break;
	}

	/* check if the physical memory needs to be allocated */
	if (phys == pPhysical.size()) {
		uint32_t allocate = fExpandPhysical(size, growth);
		if (allocate == 0)
			return phys;

		/* add the newly allocated physical memory/slot */
		if (!pPhysical[phys - 1].used)
			pPhysical[--phys].size += allocate;
		else
			pPhysical.push_back(detail::MemoryPhysical{ pPhysical.back().physical + pPhysical.back().size, allocate, false });
	}
	return phys;
}
bool env::Memory::fMemAllocateIntermediate(size_t virt, uint32_t size, uint32_t usage) {
	/* check if the previous and next block already lie aligned in physical memory */
	if (pVirtual[virt].physical + pVirtual[virt].size + size != pVirtual[virt + 1].physical)
		return false;

	/* check if the intermediate physical block is not yet allocated */
	size_t phys = fLookupPhysical(pVirtual[virt].physical);
	if (pPhysical[phys + 1].used || pPhysical[phys + 1].size != size)
		return false;

	/* merge the three physical ranges back together */
	pPhysical[phys].size += pPhysical[phys + 1].size + pPhysical[phys + 2].size;
	pPhysical.erase(pPhysical.begin() + phys + 1, pPhysical.begin() + phys + 3);

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
env::detail::physical_t env::Memory::fMemMergePhysical(size_t virt, size_t phys, uint32_t size, size_t physPrev, size_t physNext) {
	uint32_t capacity = pPhysical[phys].size;

	/* check which sides exist */
	bool hasPrev = (physPrev < pPhysical.size());
	bool hasNext = (physNext < pPhysical.size());

	/* mark the new physical slot as used and initialize its size */
	pPhysical[phys].size = size;
	pPhysical[phys].used = true;

	/* move the original regions into place */
	detail::physical_t actual = pPhysical[phys].physical;
	if (hasPrev) {
		/* move the memory into place and patch the old and new physical slot */
		fMovePhysical(actual, pPhysical[physPrev].physical, pPhysical[physPrev].size);
		pPhysical[phys].size += pPhysical[physPrev].size;
		pPhysical[physPrev].used = false;

		/* patch the virtual slots */
		uint32_t offset = pPhysical[physPrev].size;
		for (size_t i = virt; offset > 0; --i) {
			offset -= pVirtual[i].size;
			pVirtual[i].physical = actual + offset;
		}
		actual += pPhysical[physPrev].size;
	}
	if (hasNext) {
		/* move the memory into place and patch the old and new physical slot */
		fMovePhysical(actual + size, pPhysical[physNext].physical, pPhysical[physNext].size);
		pPhysical[phys].size += pPhysical[physNext].size;
		pPhysical[physNext].used = false;

		/* patch the virtual slots */
		uint32_t offset = 0;
		for (size_t i = virt + 1; offset < pPhysical[physNext].size; ++i) {
			pVirtual[i].physical = actual + size + offset;
			offset += pVirtual[i].size;
		}
	}

	/* check if capacity remains, which needs to be written to a new slot */
	capacity -= pPhysical[phys].size;
	if (capacity > 0) {
		detail::physical_t end = pPhysical[phys].physical + pPhysical[phys].size;

		/* check if an existing slot can be expanded */
		if (phys + 1 < pPhysical.size() && !pPhysical[phys + 1].used)
			pPhysical[phys + 1] = detail::MemoryPhysical{ end, pPhysical[phys + 1].size + capacity, false };

		/* allocate or insert the new physical slot */
		else
			pPhysical.insert(pPhysical.begin() + phys + 1, detail::MemoryPhysical{ end, capacity, false });
	}

	/* merge any remaining empty slots and remove duplicates */
	size_t last = std::min<size_t>((hasPrev ? physPrev : pPhysical.size()), (hasNext ? physNext : pPhysical.size()));
	if (last > 0)
		--last;
	for (size_t i = last + 1; i < pPhysical.size(); ++i) {
		if (!pPhysical[last].used && !pPhysical[i].used)
			pPhysical[last].size += pPhysical[i].size;
		else if (++last < i)
			pPhysical[last] = pPhysical[i];
	}
	if (++last < pPhysical.size())
		pPhysical.resize(last);
	return actual;
}

void env::Memory::fMemUnmapSingleBlock(size_t virt, env::guest_t address, uint32_t size) {
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
	uint32_t offset = uint32_t(address - entry.address);
	pVirtual[virt].size = offset;
	offset += size;
	pVirtual.insert(pVirtual.begin() + virt + 1, detail::MemoryVirtual{ address + size, entry.physical + offset, entry.size - offset, entry.usage });
}
void env::Memory::fMemUnmapMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end) {
	uint32_t shift = 0;

	/* check if the first slot needs to be split up */
	if (pVirtual[virt].address < address) {
		shift = uint32_t(pVirtual[virt].address + pVirtual[virt].size - address);
		pVirtual[virt].size = uint32_t(address - pVirtual[virt].address);
		++virt;
	}

	/* remove all remaining blocks until the end has been reached */
	size_t dropped = 0;
	while (true) {
		/* check if the next virtual slot is valid */
		size_t next = virt + dropped;
		if (next >= pVirtual.size() || pVirtual[next].address != address + shift)
			host::Fatal(u8"Unmapping range is not fully mapped");

		/* check if the slot is fully unmapped */
		if (pVirtual[next].address + pVirtual[next].size <= end) {
			++dropped;
			shift = 0;

			/* check if this was the last block */
			if (pVirtual[next].address + pVirtual[next].size == end)
				break;
			address += pVirtual[next].size;
			continue;
		}

		/* patch the last block to only contain the remainder */
		uint32_t offset = uint32_t(end - pVirtual[next].address);
		pVirtual[next].address += offset;
		pVirtual[next].physical += offset;
		pVirtual[next].size -= offset;
		break;
	}

	/* drop all cleared blocks */
	if (dropped > 0)
		pVirtual.erase(pVirtual.begin() + virt, pVirtual.begin() + virt + dropped);
}
void env::Memory::fMemUnmapPhysical(size_t phys, uint32_t offset, uint32_t size) {
	/* check if the entire physical range is being removed */
	if (offset == 0 && pPhysical[phys].size == size) {
		pPhysical[phys].used = false;

		/* check if it can be merged with the next range */
		size_t dropped = 0;
		if (phys + 1 < pPhysical.size() && !pPhysical[phys + 1].used) {
			pPhysical[phys].size += pPhysical[phys + 1].size;
			++dropped;
		}

		/* check if it can be merged with the previous range */
		if (phys > 0 && !pPhysical[phys - 1].used) {
			pPhysical[phys - 1].size += pPhysical[phys].size;
			--phys;
			++dropped;
		}

		/* remove all dropped physical slots */
		if (dropped > 0)
			pPhysical.erase(pPhysical.begin() + phys + 1, pPhysical.begin() + phys + 1 + dropped);
		return;
	}

	/* check if the physical range aligns with the upper bound */
	if (offset + size == pPhysical[phys].size) {
		pPhysical[phys].size -= size;

		/* check if it can be merged with the next slot */
		if (phys + 1 < pPhysical.size() && !pPhysical[phys + 1].used) {
			pPhysical[phys + 1].physical -= size;
			pPhysical[phys + 1].size += size;
			return;
		}

		/* insert the new intermediate block */
		pPhysical.insert(pPhysical.begin() + phys + 1, detail::MemoryPhysical{ pPhysical[phys].physical + pPhysical[phys].size, size, false });
		return;
	}

	/* check if the physical range aligns with the lower bound */
	if (offset == 0) {
		pPhysical[phys].physical += size;
		pPhysical[phys].size -= size;

		/* check if it can be merged with the previous slot */
		if (phys > 0 && !pPhysical[phys - 1].used) {
			pPhysical[phys - 1].size += size;
			return;
		}

		/* insert the new intermediate block */
		pPhysical.insert(pPhysical.begin() + phys, detail::MemoryPhysical{ pPhysical[phys].physical - size, size, false });
		return;
	}

	/* insert the two new blocks and populate them */
	pPhysical.insert(pPhysical.begin() + phys + 1, 2, detail::MemoryPhysical{});
	pPhysical[phys + 1] = detail::MemoryPhysical{ pPhysical[phys].physical + offset, size, false };
	size += offset;
	pPhysical[phys + 2] = detail::MemoryPhysical{ pPhysical[phys].physical + size, pPhysical[phys].size - size, true };
	pPhysical[phys].size = offset;
}

bool env::Memory::fMemProtectSingleBlock(size_t virt, env::guest_t address, uint32_t size, uint32_t usage) {
	const detail::MemoryVirtual entry = pVirtual[virt];

	/* check if nothing needs to be done */
	if (entry.usage == usage)
		return false;

	/* check if a previous or next block exists */
	bool hasPrev = (virt > 0 && pVirtual[virt - 1].address + uint64_t(pVirtual[virt - 1].size) == entry.address);
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
		return true;
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
		return true;
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
		return true;
	}

	/* a new slot needs to be inserted below and above (no possibility to merge with neighboring blocks) */
	pVirtual.insert(pVirtual.begin() + virt + 1, 2, detail::MemoryVirtual{});
	uint32_t offset = uint32_t(address - entry.address);
	pVirtual[virt].size = offset;
	pVirtual[virt + 1] = detail::MemoryVirtual{ address, entry.physical + offset, size, usage };
	pVirtual[virt + 2] = detail::MemoryVirtual{ address + size, entry.physical + offset + size, entry.size - offset - size, entry.usage };
	return true;
}
void env::Memory::fMemProtectMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end, uint32_t size, uint32_t usage) {
	bool hasValue = false;
	uint32_t shift = 0;

	/* check if the first slot needs to be split up or if it can be used directly */
	detail::physical_t start = pVirtual[virt].physical;
	if (pVirtual[virt].address < address) {
		if (pVirtual[virt].usage != usage) {
			shift = uint32_t(pVirtual[virt].address + pVirtual[virt].size - address);
			pVirtual[virt].size = uint32_t(address - pVirtual[virt].address);
			start += pVirtual[virt].size;
		}
		else {
			address = pVirtual[virt].address + pVirtual[virt].size;
			start = pPhysical[virt].physical + pVirtual[virt].size;
			hasValue = true;
		}
		++virt;
	}

	/* check if the previous block already matches the type */
	else if (virt > 0 && pVirtual[virt - 1].address + uint64_t(pVirtual[virt - 1].size) == address && pVirtual[virt - 1].usage == usage)
		hasValue = true;

	/* convert all remaining blocks until the end has been reached */
	size_t dropped = 0;
	while (true) {
		/* check if the next virtual slot is valid */
		size_t next = virt + dropped;
		if (next >= pVirtual.size() || pVirtual[next].address != address + shift)
			host::Fatal(u8"Change range is not fully mapped");

		/* check if the slot is fully mapped across */
		if (pVirtual[next].address + pVirtual[next].size <= end) {
			/* check if the slot can just be added to the previous slot */
			if (hasValue) {
				pVirtual[virt - 1].size += pVirtual[next].size;
				++dropped;
			}

			/* make this the initial slot */
			else {
				pVirtual[next] = detail::MemoryVirtual{ address, start, pVirtual[next].size + shift, usage };
				hasValue = true;
				shift = 0;
				++virt;
			}

			/* check if this was the last block */
			if (pVirtual[next].address + pVirtual[next].size == end)
				break;
			address += pVirtual[next].size;
			continue;
		}
		size = uint32_t(end - address);
		uint32_t remainderUsage = pVirtual[next].usage;
		uint32_t remainderSize = shift + pVirtual[next].size - size;

		/* check if the last slot is of the same usage and can just be added */
		if (remainderUsage == usage) {
			if (hasValue) {
				pVirtual[virt - 1].size += pVirtual[next].size;
				++dropped;
			}
			else
				pVirtual[next] = { address, start, size + remainderSize, usage };
		}

		/* add the remaining capacity to the previous slot and clip it from the next slot */
		else if (hasValue) {
			pVirtual[virt - 1].size += size;
			pVirtual[next].address += size;
			pVirtual[next].physical += size;
			pVirtual[next].size -= size;
		}

		/* use the new block as initial block and insert the remainder as a new block */
		else {
			pVirtual[next] = detail::MemoryVirtual{ address, start, size, usage };
			pVirtual.insert(pVirtual.begin() + next + 1, detail::MemoryVirtual{ address + size, start + size, remainderSize, remainderUsage });
		}
		break;
	}

	/* drop all cleared blocks */
	if (dropped > 0)
		pVirtual.erase(pVirtual.begin() + virt, pVirtual.begin() + virt + dropped);
}

void env::Memory::fCacheLookup(env::guest_t address, env::guest_t access, uint32_t size, uint32_t usage, uint32_t cache) const {
	host::Debug(str::u8::Format(u8"Lookup [{:#018x}] with size [{}] and usage [{}{}{}] from [{:#018x}]", access, size,
		(usage & env::MemoryUsage::Read ? u8'r' : u8'-'),
		(usage & env::MemoryUsage::Write ? u8'w' : u8'-'),
		(usage & env::MemoryUsage::Execute ? u8'x' : u8'-'),
		address
	));
	detail::MemoryLookup lookup = fLookup(address, access, size, usage);

	/* write the lookup back to the cache */
	pCaches[cache] = {
		lookup.address,
		lookup.physical,
		lookup.size - 1,
		lookup.size - 2,
		lookup.size - 4,
		lookup.size - 8
	};
}
uint64_t env::Memory::fRead(env::guest_t address, uint32_t size) const {
	return detail::MemoryBridge::Read(address, size);
}
void env::Memory::fWrite(env::guest_t address, uint32_t size, uint64_t value) const {
	detail::MemoryBridge::Write(address, size, value);
}
uint64_t env::Memory::fCode(env::guest_t address, uint32_t size) const {
	return detail::MemoryBridge::Code(address, size);
}

bool env::Memory::mmap(env::guest_t address, uint32_t size, uint32_t usage) {
	host::Debug(str::u8::Format(u8"Mapping [{:#018x}] with size [{:#010x}] and usage [{}{}{}]", address, size,
		(usage & env::MemoryUsage::Read ? u8'r' : u8'-'),
		(usage & env::MemoryUsage::Write ? u8'w' : u8'-'),
		(usage & env::MemoryUsage::Execute ? u8'x' : u8'-')
	));

	/* check if the address and size are aligned properly and validate the usage */
	if (fPageOffset(address) != 0 || fPageOffset(size) != 0)
		host::Fatal(u8"Mapping requires address and size to be page-aligned");
	if ((usage & ~(env::MemoryUsage::Read | env::MemoryUsage::Write | env::MemoryUsage::Execute)) != 0)
		host::Fatal(u8"Memory-usage must only consist of read/write/execute usage");

	/* check if the given range is valid and does not overlap any existing ranges */
	size_t virt = fLookupVirtual(address);
	if (virt < pVirtual.size() && address >= pVirtual[virt].address && address < pVirtual[virt].address + pVirtual[virt].size)
		host::Fatal(u8"Mapping range is already partially mapped");
	if (virt + 1 < pVirtual.size() && address + size > pVirtual[virt + 1].address)
		host::Fatal(u8"Mapping range is already partially mapped");
	if (size == 0)
		return true;

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

	/* compute the size of the entire contiguous range */
	size_t physPrev = (hasPrev ? fLookupPhysical(pVirtual[virt].physical) : pPhysical.size());
	size_t physNext = (hasNext ? fLookupPhysical(pVirtual[virt + 1].physical) : pPhysical.size());

	/* lookup a physical slot large enough to house the contiguous memory region */
	size_t phys = fMemAllocatePhysical((hasPrev ? pPhysical[physPrev].size : 0) + size + (hasNext ? pPhysical[physNext].size : 0), size);
	if (phys >= pPhysical.size()) {
		host::Debug(u8"Allocation failed");
		return false;
	}

	/* patch the phys-prev/phys-next indices (as the allocation might have changed the overall count) */
	if (!hasPrev)
		physPrev = pPhysical.size();
	if (!hasNext)
		physNext = pPhysical.size();

	/* merge the previous and next blocks into the new contiguous physical region (might invalidate the phys-indices) */
	detail::physical_t actual = fMemMergePhysical(virt, phys, size, physPrev, physNext);

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
	uint32_t end = pPhysical.back().physical + pPhysical.back().size;
	if (!pPhysical[0].used && pPhysical[0].size >= (end / detail::ShiftMemoryFactor)) {
		uint32_t shift = pPhysical[1].physical;
		fMovePhysical(0, shift, end - shift);

		/* move the physical and virtual memory down */
		for (size_t i = 1; i < pPhysical.size(); ++i)
			pPhysical[i - 1] = detail::MemoryPhysical{ pPhysical[i].physical - shift, pPhysical[i].size, pPhysical[i].used };
		pPhysical.pop_back();
		for (size_t i = 0; i < pVirtual.size(); ++i)
			pVirtual[i].physical -= shift;
	}

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
	return true;
}
void env::Memory::munmap(env::guest_t address, uint32_t size) {
	host::Debug(str::u8::Format(u8"Unmapping [{:#018x}] with size [{:#010x}]", address, size));

	/* check if the address and size are aligned properly */
	if (fPageOffset(address) != 0 || fPageOffset(size) != 0)
		host::Fatal(u8"Unmapping requires address and size to be page-aligned");

	/* check if the given start is valid and lies within a mapped region */
	size_t virt = fLookupVirtual(address);
	if (virt >= pVirtual.size() || address < pVirtual[virt].address)
		host::Fatal(u8"Unmapping range is not fully mapped");
	if (size == 0)
		return;

	/* cache the physical state to be able to patch it afterwards */
	size_t phys = fLookupPhysical(pVirtual[virt].physical);
	uint32_t offset = (pVirtual[virt].physical - pPhysical[phys].physical) + uint32_t(address - pVirtual[virt].address);

	/* check if the new usage lies across multiple slots */
	env::guest_t end = address + size;
	if (pVirtual[virt].address + pVirtual[virt].size < end)
		fMemUnmapMultipleBlocks(virt, address, end);

	/* apply the new usage to the single block */
	else
		fMemUnmapSingleBlock(virt, address, size);

	/* unmap the physical memory and flush the caches to ensure the new mapping is accepted */
	fMemUnmapPhysical(phys, offset, size);
	fFlushCaches();
}
void env::Memory::mprotect(env::guest_t address, uint32_t size, uint32_t usage) {
	host::Debug(str::u8::Format(u8"Changing [{:#018x}] with size [{:#010x}] and usage [{}{}{}]", address, size,
		(usage & env::MemoryUsage::Read ? u8'r' : u8'-'),
		(usage & env::MemoryUsage::Write ? u8'w' : u8'-'),
		(usage & env::MemoryUsage::Execute ? u8'x' : u8'-')
	));

	/* check if the address and size are aligned properly and validate the usage */
	if (fPageOffset(address) != 0 || fPageOffset(size) != 0)
		host::Fatal(u8"Changing requires address and size to be page-aligned");
	if ((usage & ~(env::MemoryUsage::Read | env::MemoryUsage::Write | env::MemoryUsage::Execute)) != 0)
		host::Fatal(u8"Memory-usage must only consist of read/write/execute usage");

	/* check if the given start is valid and lies within a mapped region */
	size_t virt = fLookupVirtual(address);
	if (virt >= pVirtual.size() || address < pVirtual[virt].address)
		host::Fatal(u8"Change range is not fully mapped");
	if (size == 0)
		return;

	/* check if the new usage lies across multiple slots */
	env::guest_t end = address + size;
	if (pVirtual[virt].address + pVirtual[virt].size < end)
		fMemProtectMultipleBlocks(virt, address, end, size, usage);

	/* apply the new usage to the single block */
	else if (!fMemProtectSingleBlock(virt, address, size, usage))
		return;

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
}
void env::Memory::mread(uint8_t* dest, env::guest_t source, uint32_t size, uint32_t usage) const {
	host::Debug(str::u8::Format(u8"Reading [{:#018x}] with size [{:#010x}] and usage [{}{}{}]", source, size,
		(usage & env::MemoryUsage::Read ? u8'r' : u8'-'),
		(usage & env::MemoryUsage::Write ? u8'w' : u8'-'),
		(usage & env::MemoryUsage::Execute ? u8'x' : u8'-')
	));

	/* lookup the address to ensure it is mapped and to fetch the physical address */
	detail::MemoryLookup lookup = fLookup(detail::MainAccessAddress, source, size, usage);
	detail::MemoryBridge::ReadFromPhysical(dest, lookup.physical + detail::physical_t(source - lookup.address), size);
}
void env::Memory::mwrite(env::guest_t dest, const uint8_t* source, uint32_t size, uint32_t usage) {
	host::Debug(str::u8::Format(u8"Writing [{:#018x}] with size [{:#010x}] and usage [{}{}{}]", dest, size,
		(usage & env::MemoryUsage::Read ? u8'r' : u8'-'),
		(usage & env::MemoryUsage::Write ? u8'w' : u8'-'),
		(usage & env::MemoryUsage::Execute ? u8'x' : u8'-')
	));

	/* lookup the address to ensure it is mapped and to fetch the physical address */
	detail::MemoryLookup lookup = fLookup(detail::MainAccessAddress, dest, size, usage);
	detail::MemoryBridge::WriteToPhysical(lookup.physical + detail::physical_t(dest - lookup.address), source, size);
}
void env::Memory::mclear(env::guest_t dest, uint32_t size, uint32_t usage) {
	host::Debug(str::u8::Format(u8"Clearing [{:#018x}] with size [{:#010x}] and usage [{}{}{}]", dest, size,
		(usage & env::MemoryUsage::Read ? u8'r' : u8'-'),
		(usage & env::MemoryUsage::Write ? u8'w' : u8'-'),
		(usage & env::MemoryUsage::Execute ? u8'x' : u8'-')
	));

	/* lookup the address to ensure it is mapped and to fetch the physical address */
	detail::MemoryLookup lookup = fLookup(detail::MainAccessAddress, dest, size, usage);
	detail::MemoryBridge::ClearPhysical(lookup.physical + detail::physical_t(dest - lookup.address), size);
}
