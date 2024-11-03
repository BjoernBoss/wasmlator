#include "../env-process.h"

namespace I = wasm::inst;

env::detail::MemoryMapper::MemoryMapper(env::Process* process) : pProcess{ process } {
}

size_t env::detail::MemoryMapper::fLookupVirtual(env::guest_t address) const {
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
size_t env::detail::MemoryMapper::fLookupPhysical(env::physical_t physical) const {
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

uint32_t env::detail::MemoryMapper::fExpandPhysical(uint32_t size, uint32_t growth) const {
	/* allocate a little bit more to reduce the number of growings */
	uint32_t pages = env::PhysPageCount(std::max<uint32_t>(env::MinGrowthBytes, size + growth));
	if (bridge::Memory::ExpandPhysical(pProcess->id(), pages))
		return uint32_t(pages * env::PhysPageSize);
	size = env::PhysPageCount(size);
	if (size < pages && bridge::Memory::ExpandPhysical(pProcess->id(), size))
		return uint32_t(size * env::PhysPageSize);
	return 0;
}
void env::detail::MemoryMapper::fMovePhysical(env::physical_t dest, env::physical_t source, uint32_t size) const {
	bridge::Memory::MovePhysical(pProcess->id(), dest, source, size);
}
void env::detail::MemoryMapper::fFlushCaches() const {
	fCheckConsistency();
	bridge::Memory::FlushCaches(pProcess->id());
}
void env::detail::MemoryMapper::fCheckConsistency() const {
	if (pPhysical.empty())
		pProcess->fail(u8"Physical slots invalid");

	uint32_t totalPhys = 0, totalVirt = 0;
	for (size_t i = 0; i < pPhysical.size(); ++i) {
		totalPhys += (pPhysical[i].used ? pPhysical[i].size : 0);

		if (fLookupPhysical(pPhysical[i].physical) != i)
			pProcess->fail(u8"Physical slot [", i, u8"] lookup failed");
		if (pPhysical[i].size == 0 || env::VirtPageOffset(pPhysical[i].size) != 0)
			pProcess->fail(u8"Physical slot [", i, u8"] size is invalid");

		if ((i > 0 ? (pPhysical[i - 1].physical + pPhysical[i - 1].size) : 0) != pPhysical[i].physical)
			pProcess->fail(u8"Physical slot [", i, u8"] address is invalid");
		if (i > 0 && !pPhysical[i - 1].used && !pPhysical[i].used)
			pProcess->fail(u8"Physical slot [", i, u8"] usage is invalid");
	}

	for (size_t i = 0; i < pVirtual.size(); ++i) {
		totalVirt += pVirtual[i].size;

		if (fLookupVirtual(pVirtual[i].address) != i)
			pProcess->fail(u8"Virtual slot [", i, u8"] lookup failed");

		size_t phys = fLookupPhysical(pVirtual[i].physical);
		if (phys >= pPhysical.size() || !pPhysical[phys].used || pVirtual[i].physical < pPhysical[phys].physical ||
			pVirtual[i].physical + pVirtual[i].size > pPhysical[phys].physical + pPhysical[phys].size)
			pProcess->fail(u8"Virtual slot [", i, u8"] physical is invalid");

		if (pVirtual[i].size == 0 || env::VirtPageOffset(pVirtual[i].size) != 0)
			pProcess->fail(u8"Virtual slot [", i, u8"] size is invalid");
		if (i == 0)
			continue;

		if (pVirtual[i - 1].address + pVirtual[i - 1].size > pVirtual[i].address)
			pProcess->fail(u8"Virtual slot [", i, u8"] address is invalid");
		if (pVirtual[i - 1].address + pVirtual[i - 1].size < pVirtual[i].address)
			continue;

		if (pVirtual[i - 1].physical + pVirtual[i - 1].size != pVirtual[i].physical)
			pProcess->fail(u8"Virtual slot [", i, u8"] physical is invalid");
		if (pVirtual[i - 1].usage == pVirtual[i].usage)
			pProcess->fail(u8"Virtual slot [", i, u8"] usage is invalid");
	}

	if (totalPhys != totalVirt)
		pProcess->fail(u8"Phyiscal used memory does not match virtual used memory");
	pProcess->debug(u8"Memory consistency-check performed");
}

bool env::detail::MemoryMapper::fMemExpandPrevious(size_t virt, env::guest_t address, uint32_t size, uint32_t usage) {
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
				pPhysical.push_back(MemoryMapper::MemPhysical{ 0, allocate, false });
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
		env::physical_t actual = pVirtual[virt].physical + pVirtual[virt].size;
		pVirtual.insert(pVirtual.begin() + virt + 1, MemoryMapper::MemVirtual{ address, actual, size, usage });
	}
	return true;
}
size_t env::detail::MemoryMapper::fMemAllocatePhysical(uint32_t size, uint32_t growth) {
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

		/* add the physical slot */
		pPhysical.push_back(MemoryMapper::MemPhysical{ pPhysical.back().physical + pPhysical.back().size, allocate, false });
	}
	return phys;
}
bool env::detail::MemoryMapper::fMemAllocateIntermediate(size_t virt, uint32_t size, uint32_t usage) {
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
	pVirtual.insert(pVirtual.begin() + virt + 1, MemoryMapper::MemVirtual{ pVirtual[virt].address + pVirtual[virt].size, pVirtual[virt].physical + pVirtual[virt].size, size, usage });
	return true;
}
env::physical_t env::detail::MemoryMapper::fMemMergePhysical(size_t virt, size_t phys, uint32_t size, size_t physPrev, size_t physNext) {
	uint32_t capacity = pPhysical[phys].size;

	/* check which sides exist */
	bool hasPrev = (physPrev < pPhysical.size());
	bool hasNext = (physNext < pPhysical.size());

	/* mark the new physical slot as used and initialize its size */
	pPhysical[phys].size = size;
	pPhysical[phys].used = true;

	/* move the original regions into place */
	env::physical_t actual = pPhysical[phys].physical;
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
		env::physical_t end = pPhysical[phys].physical + pPhysical[phys].size;

		/* check if an existing slot can be expanded */
		if (phys + 1 < pPhysical.size() && !pPhysical[phys + 1].used)
			pPhysical[phys + 1] = MemoryMapper::MemPhysical{ end, pPhysical[phys + 1].size + capacity, false };

		/* allocate or insert the new physical slot */
		else
			pPhysical.insert(pPhysical.begin() + phys + 1, MemoryMapper::MemPhysical{ end, capacity, false });
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

void env::detail::MemoryMapper::fMemUnmapSingleBlock(size_t virt, env::guest_t address, uint32_t size) {
	const MemoryMapper::MemVirtual entry = pVirtual[virt];

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
	pVirtual.insert(pVirtual.begin() + virt + 1, MemoryMapper::MemVirtual{ address + size, entry.physical + offset, entry.size - offset, entry.usage });
}
void env::detail::MemoryMapper::fMemUnmapMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end) {
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
			pProcess->fail(u8"Unmapping range is not fully mapped");

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
void env::detail::MemoryMapper::fMemUnmapPhysical(size_t phys, uint32_t offset, uint32_t size) {
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
		pPhysical.insert(pPhysical.begin() + phys + 1, MemoryMapper::MemPhysical{ pPhysical[phys].physical + pPhysical[phys].size, size, false });
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
		pPhysical.insert(pPhysical.begin() + phys, MemoryMapper::MemPhysical{ pPhysical[phys].physical - size, size, false });
		return;
	}

	/* insert the two new blocks and populate them */
	pPhysical.insert(pPhysical.begin() + phys + 1, 2, MemoryMapper::MemPhysical{});
	pPhysical[phys + 1] = MemoryMapper::MemPhysical{ pPhysical[phys].physical + offset, size, false };
	size += offset;
	pPhysical[phys + 2] = MemoryMapper::MemPhysical{ pPhysical[phys].physical + size, pPhysical[phys].size - size, true };
	pPhysical[phys].size = offset;
}

bool env::detail::MemoryMapper::fMemProtectSingleBlock(size_t virt, env::guest_t address, uint32_t size, uint32_t usage) {
	const MemoryMapper::MemVirtual entry = pVirtual[virt];

	/* check if nothing needs to be done */
	if (entry.usage == usage)
		return false;

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
			pVirtual.insert(pVirtual.begin() + virt + 1, MemoryMapper::MemVirtual{ entry.address + size, entry.physical + size, entry.size - size, entry.usage });
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
			pVirtual.insert(pVirtual.begin() + virt + 1, MemoryMapper::MemVirtual{ entry.address + size, entry.physical + size, entry.size - size, usage });
			pVirtual[virt].size = size;
		}
		return true;
	}

	/* a new slot needs to be inserted below and above (no possibility to merge with neighboring blocks) */
	pVirtual.insert(pVirtual.begin() + virt + 1, 2, MemoryMapper::MemVirtual{});
	uint32_t offset = uint32_t(address - entry.address);
	pVirtual[virt].size = offset;
	pVirtual[virt + 1] = MemoryMapper::MemVirtual{ address, entry.physical + offset, size, usage };
	pVirtual[virt + 2] = MemoryMapper::MemVirtual{ address + size, entry.physical + offset + size, entry.size - offset - size, entry.usage };
	return true;
}
void env::detail::MemoryMapper::fMemProtectMultipleBlocks(size_t virt, env::guest_t address, env::guest_t end, uint32_t size, uint32_t usage) {
	bool hasValue = false;
	uint32_t shift = 0;

	/* check if the first slot needs to be split up or if it can be used directly */
	env::physical_t start = pVirtual[virt].physical;
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
	else if (virt > 0 && pVirtual[virt - 1].address + pVirtual[virt - 1].size == address && pVirtual[virt - 1].usage == usage)
		hasValue = true;

	/* convert all remaining blocks until the end has been reached */
	size_t dropped = 0;
	while (true) {
		/* check if the next virtual slot is valid */
		size_t next = virt + dropped;
		if (next >= pVirtual.size() || pVirtual[next].address != address + shift)
			pProcess->fail(u8"Change range is not fully mapped");

		/* check if the slot is fully mapped across */
		if (pVirtual[next].address + pVirtual[next].size <= end) {
			/* check if the slot can just be added to the previous slot */
			if (hasValue) {
				pVirtual[virt - 1].size += pVirtual[next].size;
				++dropped;
			}

			/* make this the initial slot */
			else {
				pVirtual[next] = MemoryMapper::MemVirtual{ address, start, pVirtual[next].size + shift, usage };
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
			pVirtual[next] = MemoryMapper::MemVirtual{ address, start, size, usage };
			pVirtual.insert(pVirtual.begin() + next + 1, MemoryMapper::MemVirtual{ address + size, start + size, remainderSize, remainderUsage });
		}
		break;
	}

	/* drop all cleared blocks */
	if (dropped > 0)
		pVirtual.erase(pVirtual.begin() + virt, pVirtual.begin() + virt + dropped);
}

void env::detail::MemoryMapper::configure(uint32_t initialPageCount) {
	/* setup the physical mapping */
	pPhysical.push_back(MemoryMapper::MemPhysical{ 0, uint32_t(env::PhysPageSize * initialPageCount), false });
}

void env::detail::MemoryMapper::lookup(env::guest_t address, uint32_t size, uint32_t usage) const {
	pProcess->debug(str::Format<std::u8string>(u8"Lookup [{:#018x}] with size [{}] and usage [{}{}{}]", address, size,
		(usage & env::MemoryUsage::Read ? u8'r' : u8'-'),
		(usage & env::MemoryUsage::Write ? u8'w' : u8'-'),
		(usage & env::MemoryUsage::Execute ? u8'e' : u8'-')
	));

	/* lookup the virtual mapping containing the corresponding address */
	size_t index = fLookupVirtual(address);

	/* check if an entry has been found */
	if (index >= pVirtual.size() || address < pVirtual[index].address || address >= pVirtual[index].address + pVirtual[index].size)
		pProcess->fail(str::Format<std::u8string>(u8"Virtual page-fault at address [{:#018x}] encountered", address));

	/* check if the usage attributes are valid */
	if ((pVirtual[index].usage & usage) != usage)
		pProcess->fail(str::Format<std::u8string>(u8"Virtual page-protection fault at address [{:#018x}] encountered", address));

	/* collect all previous and upcoming regions of the same usage */
	pLastLookup = MemoryMapper::MemLookup{ pVirtual[index].address, pVirtual[index].physical, pVirtual[index].size };
	for (size_t i = index; i > 0; --i) {
		if (pVirtual[i - 1].address + pVirtual[i - 1].size != pVirtual[i].address)
			break;
		if ((pVirtual[i - 1].usage & usage) != usage)
			break;
		pLastLookup = { pVirtual[i - 1].address, pVirtual[i - 1].physical, pVirtual[index].size + pLastLookup.size };
	}
	for (size_t i = index + 1; i < pVirtual.size(); ++i) {
		if (pLastLookup.address + pLastLookup.size != pVirtual[i].address)
			break;
		if ((pVirtual[i].usage & usage) != usage)
			break;
		pLastLookup.size += pVirtual[i].size;
	}

	/* check if the access-size is valid */
	if (pLastLookup.address + pLastLookup.size < address + size)
		pProcess->fail(str::Format<std::u8string>(u8"Virtual page-fault at address [{:#018x}] encountered", address));
}
const env::detail::MemoryMapper::MemLookup& env::detail::MemoryMapper::lastLookup() const {
	return pLastLookup;
}
bool env::detail::MemoryMapper::mmap(env::guest_t address, uint32_t size, uint32_t usage) {
	pProcess->debug(str::Format<std::u8string>(u8"Mapping [{:#018x}] with size [{:#010x}] and usage [{}{}{}]", address, size,
		(usage & env::MemoryUsage::Read ? u8'r' : u8'-'),
		(usage & env::MemoryUsage::Write ? u8'w' : u8'-'),
		(usage & env::MemoryUsage::Execute ? u8'e' : u8'-')
	));

	/* check if the address and size are aligned properly and validate the usage */
	if (env::VirtPageOffset(address) != 0 || env::VirtPageOffset(size) != 0)
		pProcess->fail(u8"Mapping requires address and size to be page-aligned");
	if ((usage & ~(env::MemoryUsage::Read | env::MemoryUsage::Write | env::MemoryUsage::Execute)) != 0)
		pProcess->fail(u8"Memory-usage must only consist of read/write/execute usage");

	/* check if the given range is valid and does not overlap any existing ranges */
	size_t virt = fLookupVirtual(address);
	if (virt < pVirtual.size() && address >= pVirtual[virt].address && address < pVirtual[virt].address + pVirtual[virt].size)
		pProcess->fail(u8"Mapping range is already partially mapped");
	if (virt + 1 < pVirtual.size() && address + size > pVirtual[virt + 1].address)
		pProcess->fail(u8"Mapping range is already partially mapped");
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
		pProcess->debug(u8"Allocation failed");
		return false;
	}

	/* merge the previous and next blocks into the new contiguous physical region (might invalidate the phys-indices) */
	env::physical_t actual = fMemMergePhysical(virt, phys, size, physPrev, physNext);

	/* check if the virtual entry can be merged with the next entry */
	size_t applied = 0;
	if (hasNext && pVirtual[virt + 1].usage == usage) {
		++applied;
		size += pVirtual[virt + 1].size;
		pVirtual[virt + 1] = MemoryMapper::MemVirtual{ address, actual, size, usage };
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
	if (!pPhysical[0].used && pPhysical[0].size >= (end / env::ShiftMemoryFactor)) {
		uint32_t shift = pPhysical[1].physical;
		fMovePhysical(0, shift, end - shift);

		/* move the physical and virtual memory down */
		for (size_t i = 1; i < pPhysical.size(); ++i)
			pPhysical[i - 1] = MemoryMapper::MemPhysical{ pPhysical[i].physical - shift, pPhysical[i].size, pPhysical[i].used };
		pPhysical.pop_back();
		for (size_t i = 0; i < pVirtual.size(); ++i)
			pVirtual[i].physical -= shift;
	}

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
	return true;
}
void env::detail::MemoryMapper::munmap(env::guest_t address, uint32_t size) {
	pProcess->debug(str::Format<std::u8string>(u8"Unmapping [{:#018x}] with size [{:#010x}]", address, size));

	/* check if the address and size are aligned properly */
	if (env::VirtPageOffset(address) != 0 || env::VirtPageOffset(size) != 0)
		pProcess->fail(u8"Unmapping requires address and size to be page-aligned");

	/* check if the given start is valid and lies within a mapped region */
	size_t virt = fLookupVirtual(address);
	if (virt >= pVirtual.size() || address < pVirtual[virt].address)
		pProcess->fail(u8"Unmapping range is not fully mapped");
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
void env::detail::MemoryMapper::mprotect(env::guest_t address, uint32_t size, uint32_t usage) {
	pProcess->debug(str::Format<std::u8string>(u8"Changing [{:#018x}] with size [{:#010x}] and usage [{}{}{}]", address, size,
		(usage & env::MemoryUsage::Read ? u8'r' : u8'-'),
		(usage & env::MemoryUsage::Write ? u8'w' : u8'-'),
		(usage & env::MemoryUsage::Execute ? u8'e' : u8'-')
	));

	/* check if the address and size are aligned properly and validate the usage */
	if (env::VirtPageOffset(address) != 0 || env::VirtPageOffset(size) != 0)
		pProcess->fail(u8"Changing requires address and size to be page-aligned");
	if ((usage & ~(env::MemoryUsage::Read | env::MemoryUsage::Write | env::MemoryUsage::Execute)) != 0)
		pProcess->fail(u8"Memory-usage must only consist of read/write/execute usage");

	/* check if the given start is valid and lies within a mapped region */
	size_t virt = fLookupVirtual(address);
	if (virt >= pVirtual.size() || address < pVirtual[virt].address)
		pProcess->fail(u8"Change range is not fully mapped");
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
