#include "env-memory-mapper.h"
#include "bridge-memory.h"

namespace I = wasm::inst;

env::detail::MemoryMapper::MemoryMapper(env::Context& context, uint32_t initialAllocated) : pContext{ &context } {
	pPhysical.push_back(MemoryMapper::MemPhysical{ 0, initialAllocated, false });
}

size_t env::detail::MemoryMapper::fLookupVirtual(env::addr_t address) const {
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
	uint32_t pages = env::PhysPageCount(std::max<uint32_t>(env::MinGrothBytes, size + growth));
	if (bridge::Memory::ExpandPhysical(pContext->id(), pages))
		return uint32_t(pages * env::PhysPageSize);
	size = env::PhysPageCount(size);
	if (size < pages && bridge::Memory::ExpandPhysical(pContext->id(), size))
		return uint32_t(size * env::PhysPageSize);
	return 0;
}
void env::detail::MemoryMapper::fMovePhysical(env::physical_t dest, env::physical_t source, uint32_t size) const {
	bridge::Memory::MovePhysical(pContext->id(), dest, source, size);
}
void env::detail::MemoryMapper::fFlushCaches() const {
	bridge::Memory::FlushCaches(pContext->id());
}

int8_t env::detail::MemoryMapper::fMemExpandPrevious(size_t virt, env::addr_t address, uint32_t size, uint32_t usage) {
	size_t phys = fLookupPhysical(pVirtual[virt].physical);

	/* check if the next page is already used, in which case no expansion is possible */
	if (phys + 1 < pPhysical.size() && pPhysical[phys + 1].used)
		return -1;

	/* check if the next physical page can be consumed */
	if (phys + 1 < pPhysical.size() && pPhysical[phys + 1].size >= size) {
		pPhysical[phys].size += size;

		/* either reduce the next physical slot or remove it entirely if consumed */
		if (size < pPhysical[phys + 1].size)
			pPhysical[phys + 1].size -= size;
		else
			pPhysical.erase(pPhysical.begin() + phys + 1);
	}

	/* expand the physical memory at the end */
	else {
		/* try to expand the physical memory (may allocate more than the needed amount) */
		uint32_t needed = size - (phys + 1 < pPhysical.size() ? pPhysical[phys + 1].size : 0);
		uint32_t allocate = fExpandPhysical(needed, size);
		if (allocate == 0)
			return 0;

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

	/* flush the caches to ensure the new mapping is applied */
	fFlushCaches();
	return 1;
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
env::physical_t env::detail::MemoryMapper::fMemMergePhysical(size_t virt, size_t phys, uint32_t size, bool hasPrev, bool hasNext) {
	uint32_t capacity = pPhysical[phys].size;

	/* mark the new physical slot as used and initialize its size */
	pPhysical[phys].size = size;
	pPhysical[phys].used = true;

	/* move the original regions into place */
	env::physical_t actual = pPhysical[phys].physical;
	if (hasPrev) {
		/* move the memory into place and patch the new physical slot */
		fMovePhysical(pPhysical[phys].physical, pVirtual[virt].physical, pVirtual[virt].size);
		actual += pVirtual[virt].size;
		pPhysical[phys].size += pVirtual[virt].size;

		/* mark the physical slot as unused */
		size_t temp = fLookupPhysical(pVirtual[virt].physical);
		pPhysical[temp].used = false;
	}
	if (hasNext) {
		/* move the memory into place and patch the new physical slot */
		fMovePhysical(actual + size, pVirtual[virt + 1].physical, pVirtual[virt + 1].size);
		pPhysical[phys].size += pVirtual[virt + 1].size;

		/* mark the physical slot as unused */
		size_t temp = fLookupPhysical(pVirtual[virt + 1].physical);
		pPhysical[temp].used = false;
	}

	/* check if capacity remains, which needs to be written to a new slot */
	capacity -= pPhysical[phys].size;
	if (capacity > 0) {
		env::physical_t end = pPhysical[phys].physical + pPhysical[phys].size;

		/* check if an existing slot can be expanded */
		if (phys + 1 < pPhysical.size() && !pPhysical[phys + 1].used)
			pPhysical[phys + 1] = MemoryMapper::MemPhysical{ end, pPhysical[phys + 1].size + capacity, false };

		/* allocate or insert the new physical slot */
		else if (phys + 1 >= pPhysical.size())
			pPhysical.push_back(MemoryMapper::MemPhysical{ end, capacity, false });
		else
			pPhysical.insert(pPhysical.begin() + phys + 1, MemoryMapper::MemPhysical{ end, capacity, false });
	}

	/* merge any remaining empty slots and remove duplicates */
	if (!hasPrev && !hasNext)
		return actual;
	size_t last = 0;
	for (size_t i = 1; i < pPhysical.size(); ++i) {
		if (!pPhysical[last].used && !pPhysical[i].used)
			pPhysical[last].size += pPhysical[i].size;
		else if (++last < i)
			pPhysical[last] = pPhysical[i];
	}
	if (++last < pPhysical.size())
		pPhysical.resize(pPhysical.size() - last);
	return actual;
}

void env::detail::MemoryMapper::fMemProtectSingleBlock(size_t virt, env::addr_t address, uint32_t size, uint32_t usage) {
	const MemoryMapper::MemVirtual entry = pVirtual[virt];

	/* check if nothing needs to be done */
	if (entry.usage == usage)
		return;

	/* check if a previous or next block exists */
	bool hasPrev = (virt > 0 && pVirtual[virt - 1].address + pVirtual[virt - 1].size == entry.address);
	bool hasNext = (virt + 1 < pVirtual.size() && pVirtual[virt + 1].address == entry.address + entry.size);

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
	}

	/* check if they align at the lower bound, in which case only one new entry needs to be added */
	else if (entry.address == address) {
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
	}

	/* check if they align at the upper bound, in which case only one new entry needs to be inserted */
	else if (entry.address + entry.size == address + size) {
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
	}

	/* a new slot needs to be inserted below and above (no possibility to merge with neighboring blocks) */
	else {
		pVirtual.insert(pVirtual.begin() + virt + 1, 2, MemoryMapper::MemVirtual{});
		uint32_t offset = uint32_t(address - entry.address);
		pVirtual[virt].size = offset;
		pVirtual[virt + 1] = MemoryMapper::MemVirtual{ address, entry.physical + offset, size, usage };
		pVirtual[virt + 2] = MemoryMapper::MemVirtual{ address + size, entry.physical + offset + size, entry.size - offset - size, entry.usage };
	}

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
}
void env::detail::MemoryMapper::fMemProtectMultipleBlocks(size_t virt, env::addr_t address, env::addr_t end, uint32_t size, uint32_t usage) {
	bool hasValue = false;
	uint32_t shift = 0;

	/* check if the first slot needs to be split up */
	env::physical_t start = pVirtual[virt].physical;
	if (pVirtual[virt].address < address) {
		shift = uint32_t(pVirtual[virt].address + pVirtual[virt].size - address);
		pVirtual[virt].size = uint32_t(address - pVirtual[virt].address);
		start += pVirtual[virt].size;
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
			pContext->fail(u8"MProtect range is not fully mapped");

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
			}

			/* check if this was the last block */
			if (pVirtual[next].address + pVirtual[next].size == end)
				break;
			address += pVirtual[next].size;
			shift = 0;
			continue;
		}
		size = uint32_t(end - address);
		uint32_t remainderUsage = pVirtual[next].usage;
		uint32_t remainderSize = shift + pVirtual[next].size - size;

		/* add the remaining capacity to the previous slot and clip it from the next slot */
		if (hasValue) {
			pVirtual[virt - 1].size += size;
			pVirtual[next].address += size;
			pVirtual[next].physical += size;
			pVirtual[next].size -= size;
		}

		/* use the new block as initial block and insert the remainder as a new block */
		else {
			pVirtual[next] = MemoryMapper::MemVirtual{ address, start, size, usage };
			pVirtual.insert(pVirtual.begin() + next + 1, MemoryMapper::MemVirtual{ address + size, start + size, remainderSize , remainderUsage });
		}
		break;
	}

	/* drop all cleared blocks */
	if (dropped > 0)
		pVirtual.erase(pVirtual.begin() + virt, pVirtual.begin() + virt + dropped);

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
}

void env::detail::MemoryMapper::lookup(env::addr_t address, uint32_t size, uint32_t usage) const {
	pContext->log(str::Format<std::u8string>(u8"Lookup [{:#16x}] with size [{}] and usage [{:04b}]", address, size, usage));

	/* lookup the virtual mapping containing the corresponding address */
	size_t index = fLookupVirtual(address);

	/* check if an entry has been found */
	if (index >= pVirtual.size() || address < pVirtual[index].address || address >= pVirtual[index].address + pVirtual[index].size)
		pContext->fail(str::Format<std::u8string>(u8"Virtual page-fault at address [{:#16x}] encountered", address));

	/* check if the usage attributes are valid */
	if ((pVirtual[index].usage & usage) != usage)
		pContext->fail(str::Format<std::u8string>(u8"Virtual page-protection fault at address [{:#16x}] encountered", address));

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
		pContext->fail(str::Format<std::u8string>(u8"Virtual page-fault at address [{:#16x}] encountered", address));
}
const env::detail::MemoryMapper::MemLookup& env::detail::MemoryMapper::lastLookup() const {
	return pLastLookup;
}
bool env::detail::MemoryMapper::mmap(env::addr_t address, uint32_t size, uint32_t usage) {
	pContext->log(str::Format<std::u8string>(u8"MMap     [{:#16x}] with size [{}] and usage [{:04b}]", address, size, usage));

	/* check if the address and size are aligned properly and validate the usage */
	if (env::VirtPageOffset(address) != 0 || env::VirtPageOffset(size) != 0)
		pContext->fail(u8"MMap requires address and size to be page-aligned");
	if ((usage & ~(env::MemoryUsage::Read | env::MemoryUsage::Write | env::MemoryUsage::Execute)) != 0)
		pContext->fail(u8"Memory-usage must only consist of read/write/execute usage");

	/* check if the given range is valid and does not overlap any existing ranges */
	size_t virt = fLookupVirtual(address);
	if (virt < pVirtual.size() && address >= pVirtual[virt].address && address < pVirtual[virt].address + pVirtual[virt].size)
		pContext->fail(u8"MMap range is already partially mapped");
	if (virt + 1 < pVirtual.size() && address + size >= pVirtual[virt + 1].address)
		pContext->fail(u8"MMap range is already partially mapped");
	if (size == 0)
		return true;

	/* check if the previous or neighboring virtual page is already allocated */
	bool hasPrev = (!pVirtual.empty() && pVirtual[virt].address + pVirtual[virt].size == address);
	bool hasNext = (virt + 1 < pVirtual.size() && pVirtual[virt + 1].address + pVirtual[virt + 1].size == address);

	/* check if an existing neighboring region can just be expanded */
	if (hasPrev && !hasNext) {
		int8_t res = fMemExpandPrevious(virt, address, size, usage);
		if (res >= 0)
			return (res > 0);
	}

	/* lookup a physical slot large enough to house the contiguous memory region */
	size_t phys = fMemAllocatePhysical((hasPrev ? pVirtual[virt].size : 0) + size + (hasNext ? pVirtual[virt + 1].size : 0), size);
	if (phys >= pPhysical.size())
		return false;

	/* merge the previous and next blocks into the new contiguous physical region (might invalidate the phys-index) */
	env::physical_t actual = fMemMergePhysical(virt, phys, size, hasPrev, hasNext);

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
	if (applied == 0)
		pVirtual.insert(pVirtual.begin() + virt + (hasPrev ? 1 : 0), { address, actual, size, usage });
	else if (applied == 2)
		pVirtual.erase(pVirtual.begin() + virt + 1);

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
	return true;
}
void env::detail::MemoryMapper::munmap(env::addr_t address, uint32_t size) {
	pContext->log(str::Format<std::u8string>(u8"MUnmap   [{:#16x}] with size [{}]", address, size));

	pContext->fail(u8"[munmap] is not implemented");
}
void env::detail::MemoryMapper::mprotect(env::addr_t address, uint32_t size, uint32_t usage) {
	pContext->log(str::Format<std::u8string>(u8"MProtect [{:#16x}] with size [{}] and usage [{:04b}]", address, size, usage));

	/* check if the address and size are aligned properly and validate the usage */
	if (env::VirtPageOffset(address) != 0 || env::VirtPageOffset(size) != 0)
		pContext->fail(u8"MProtect requires address and size to be page-aligned");
	if ((usage & ~(env::MemoryUsage::Read | env::MemoryUsage::Write | env::MemoryUsage::Execute)) != 0)
		pContext->fail(u8"Memory-usage must only consist of read/write/execute usage");

	/* check if the given start is valid and lies within a mapped region */
	size_t virt = fLookupVirtual(address);
	if (virt >= pVirtual.size() || address < pVirtual[virt].address)
		pContext->fail(u8"MProtect range is not fully mapped");
	if (size == 0)
		return;

	/* check if the entire range lies within a single block */
	env::addr_t end = address + size;
	if (pVirtual[virt].address + pVirtual[virt].size >= end)
		fMemProtectSingleBlock(virt, address, size, usage);

	/* apply the new usage across a range of slots */
	else
		fMemProtectMultipleBlocks(virt, address, end, size, usage);
}
