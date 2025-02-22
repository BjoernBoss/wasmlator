#include "../system.h"

static util::Logger logger{ u8"sys::syscall" };

env::guest_t sys::detail::MemoryInteract::fPageOffset(env::guest_t address) const {
	return (address & (pPageSize - 1));
}
env::guest_t sys::detail::MemoryInteract::fPageAlignUp(env::guest_t address) const {
	return ((address + pPageSize - 1) & ~(pPageSize - 1));
}
bool sys::detail::MemoryInteract::fCheckRange(env::guest_t address, uint64_t length, bool replace) {
	std::pair<env::guest_t, uint64_t> range = { address, 0 };
	env::guest_t end = (address + length);

	/* iterate over all allocations within the range */
	while (true) {
		range = env::Instance()->memory().findNext(range.first + range.second);

		/* check if the end has been reached, or if the mapping lies outside of the checked range */
		if (range.second == 0 || range.first >= end)
			break;

		/* check if the mapping can be removed */
		if (!replace)
			return false;

		/* patch the range to unmap all of the memory, which overlaps with the source paramter */
		uint64_t tBegin = std::max<uint64_t>(range.first, address);
		uint64_t tSize = std::min<uint64_t>(range.first + range.second, end) - tBegin;
		if (!env::Instance()->memory().munmap(tBegin, tSize))
			logger.fatal(u8"Failed to unmap existing range [", str::As{ U"#018x", tBegin }, u8"] - [", str::As{ U"#018x", tBegin + tSize - 1 }, u8']');

		/* check if this was the last range */
		if (range.first + range.second >= end)
			break;
	}
	return true;
}
int64_t sys::detail::MemoryInteract::fMapRange(env::guest_t address, uint64_t length, uint32_t usage, uint32_t flags) {
	/* check if a any address can be picked */
	if (!detail::IsSet(flags, consts::mmFlagFixed) && !detail::IsSet(flags, consts::mmFlagFixedNoReplace)) {
		address = env::Instance()->memory().alloc(length, usage);
		if (address == 0)
			return errCode::eNoMemory;
	}

	/* ensure that the address cannot be interpreted as a negative number (otherwise no distinction between errors possible) */
	else if (int64_t(address) < 0)
		return errCode::eNoMemory;

	/* first check if the range is empty or needs to be cleared */
	else if (!fCheckRange(address, length, !detail::IsSet(flags, consts::mmFlagFixedNoReplace)))
		return errCode::eExists;

	/* allocate the requested range */
	else if (!env::Instance()->memory().mmap(address, length, usage))
		return errCode::eNoMemory;
	return address;
}

bool sys::detail::MemoryInteract::setup(detail::Syscall* syscall, env::guest_t endOfData) {
	pSyscall = syscall;

	/* configure the initial break state */
	pBrk = { endOfData, endOfData, endOfData };
	pPageSize = env::Instance()->pageSize();

	/* validate the alignment of the break */
	if ((pBrk.aligned & (pPageSize - 1)) != 0) {
		logger.error(u8"Initial break address [", str::As{ U"#018x", pBrk.aligned }, u8"] is not page aligned to [", str::As{ U"#010x", pPageSize }, u8']');
		return false;
	}
	return true;
}
int64_t sys::detail::MemoryInteract::brk(env::guest_t address) {
	/* check if the address lies beneath the initial address, in which case
	*	the current break can just be returned, as no changes will be made */
	if (address < pBrk.init)
		return pBrk.current;

	/* page-align the actual address to determine if allocations need to be made or not */
	env::guest_t aligned = fPageAlignUp(address);

	/* check if memory can be released (ignore failure of unmapping the memory) */
	if (aligned < pBrk.aligned) {
		if (!env::Instance()->memory().munmap(aligned, pBrk.aligned - aligned))
			logger.warn(u8"Unable to release break-memory");
		else
			pBrk.aligned = aligned;
	}

	/* check if memory needs to be allocated */
	else if (aligned > pBrk.aligned) {
		if (!env::Instance()->memory().mmap(pBrk.aligned, aligned - pBrk.aligned, env::Usage::ReadWrite)) {
			logger.warn(u8"Unable to allocate break-memory");
			return pBrk.current;
		}
		pBrk.aligned = aligned;
	}

	/* update the current break-address to match the requested address */
	pBrk.current = address;
	return pBrk.current;
}
int64_t sys::detail::MemoryInteract::mmap(env::guest_t address, uint64_t length, uint32_t protect, uint32_t flags, int64_t fd, uint64_t offset) {
	/* check if all flags are supported */
	if ((protect & ~consts::mmProtMask) != 0)
		logger.fatal(u8"Unknown protection used in mmap");
	if ((flags & ~consts::mmFlagMask) != 0) {
		if (detail::IsSet(flags, consts::mmFlagSharedValidate))
			return errCode::eOpNotSupported;
		logger.fatal(u8"Unknown flags used in mmap");
	}

	/* validate the parameter */
	if (fPageOffset(address) != 0 || fPageOffset(offset) != 0 || length == 0)
		return errCode::eInvalid;
	if (detail::IsSet(flags, consts::mmFlagSync))
		return errCode::eOpNotSupported;

	/* note shared/shared-validate and private cannot be encoded at the same time */
	bool shared = (detail::IsSet(flags, consts::mmFlagSharedValidate) || detail::IsSet(flags, consts::mmFlagShared));
	if (shared) {
		logger.error(u8"Shared memory is currently not supported");
		return errCode::eNotImplemented;
	}

	/* map the protection to usage */
	uint32_t usage = 0;
	if (detail::IsSet(protect, consts::mmProtRead))
		usage |= env::Usage::Read;
	if (detail::IsSet(protect, consts::mmProtWrite))
		usage |= env::Usage::Write;
	if (detail::IsSet(protect, consts::mmProtExec))
		usage |= env::Usage::Execute;

	/* compute the page-aligned length */
	uint64_t alignedLength = fPageAlignUp(length);

	/* check if a simple anonymous allocation can be made */
	if (detail::IsSet(flags, consts::mmFlagAnonymous)) {
		/* validate the null-value of the file-descriptor */
		if (fd != -1)
			return errCode::eInvalid;

		/* map the range and check if an error occurred */
		int64_t result = fMapRange(address, alignedLength, usage, flags);
		if (result < 0)
			return result;
		return result;
	}

	/* validate the validity of the file-descriptor */
	detail::FdState state = pSyscall->files().fdCheck(fd);
	if (!state.valid || !state.modify)
		return errCode::eBadFd;
	if (state.type != env::FileType::file || !state.read)
		return errCode::eAccess;
	if (detail::IsSet(protect, consts::mmProtWrite)) {
		if ((shared && !state.write) || state.append)
			return errCode::eAccess;
	}

	/* check if the file should be shared (not supported for-now) */
	if (shared) {
		logger.error(u8"MMap trying to share mapped file - also not verified in mprotect");
		return errCode::eNoDevice;
	}

	/* fetch the file-states */
	return pSyscall->files().fdStats(fd, [this, fd, offset, length, flags, address, alignedLength, usage](int64_t result, const env::FileStats& stats) -> int64_t {
		/* check if an error occurred */
		if (result != errCode::eSuccess)
			return result;

		/* compute the actual number of bytes to be mapped and the corresponding page-count */
		uint64_t actual = length;
		if (offset >= stats.size)
			actual = 0;
		else if (offset + length >= stats.size)
			actual = stats.size - offset;

		/* map the range and check if an error occurred */
		int64_t allocated = fMapRange(address, alignedLength, usage, flags);
		if (allocated < 0)
			return allocated;

		/* read the file and write the data to the range */
		return pSyscall->files().fdRead(fd, offset, actual, [this, allocated, alignedLength](const uint8_t* ptr, uint64_t size) -> int64_t {
			/* write the received data to the guest */
			if (size > 0)
				env::Instance()->memory().mwrite(allocated, ptr, size, env::Usage::None);

			/* mark the remaining pages as locked (i.e. update the mapping to highlight its not part of the
			*	file - first after the entire range has been allocated to ensure the allocation succeeded) */
			uint64_t alignedSize = fPageAlignUp(size);
			if (alignedSize < alignedLength && !env::Instance()->memory().mprotect(allocated + alignedSize, alignedLength - alignedSize, env::Usage::Lock))
				logger.fatal(u8"Unexpected error while locking remainder of allocated range for mmap");

			/* return the finally allocated and mapped size */
			return allocated;
			});
		});
}
int64_t sys::detail::MemoryInteract::mprotect(env::guest_t address, uint64_t length, uint32_t protect) {
	/* check if all flags are supported */
	if ((protect & ~consts::mmProtMask) != 0)
		logger.fatal(u8"Unknown protection used in mprotect");

	/* check if nothing needs to be done */
	if (length == 0)
		return errCode::eSuccess;

	/* validate the parameter */
	if (fPageOffset(address) != 0)
		return errCode::eInvalid;

	/* map the protection to usage */
	uint32_t usage = 0;
	if (detail::IsSet(protect, consts::mmProtRead))
		usage |= env::Usage::Read;
	if (detail::IsSet(protect, consts::mmProtWrite))
		usage |= env::Usage::Write;
	if (detail::IsSet(protect, consts::mmProtExec))
		usage |= env::Usage::Execute;

	/* perform the memory operation */
	if (!env::Instance()->memory().mprotect(address, fPageAlignUp(length), usage))
		return errCode::eNoMemory;
	return errCode::eSuccess;
}
int64_t sys::detail::MemoryInteract::munmap(env::guest_t address, uint64_t length) {
	/* check if nothing needs to be done */
	if (length == 0)
		return errCode::eSuccess;

	/* validate the parameter */
	if (fPageOffset(address) != 0)
		return errCode::eInvalid;

	/* perform the memory operation */
	if (!env::Instance()->memory().munmap(address, fPageAlignUp(length)))
		return errCode::eNoMemory;
	return errCode::eSuccess;
}
int64_t sys::detail::MemoryInteract::mremap(env::guest_t old_addr, uint64_t old_len, uint64_t new_len, uint32_t flags, env::guest_t new_addr) {
	/* check if all flags are supported */
	if ((flags & ~consts::mmvMask) != 0) {
		logger.error(u8"Unknown flags used in mremap");
		return errCode::eInvalid;
	}

	/* validate the paramter */
	env::guest_t end = fPageAlignUp(old_addr + old_len);
	if (fPageOffset(old_addr) != 0 || end <= old_addr || new_len == 0)
		return errCode::eInvalid;

	/* ensure that the entire source region lies in a single mapping */
	std::pair<env::guest_t, uint64_t> range = env::Instance()->memory().findNext(old_addr);
	if (range.first > old_addr || range.first + range.second < old_addr + old_len)
		return errCode::eFault;
	uint32_t usage = env::Instance()->memory().getUsage(old_addr);

	/* check if the memory should be increased in-place */
	if (!detail::IsSet(flags, consts::mmvMayMove)) {
		if (detail::IsSet(flags, consts::mmvFixed) || detail::IsSet(flags, consts::mmvDontUnmap))
			return errCode::eInvalid;

		/* check if the allocation should be decrased in size */
		env::guest_t nend = fPageAlignUp(old_addr + new_len);
		if (nend == end)
			return old_addr;
		if (nend < end) {
			if (!env::Instance()->memory().munmap(nend, end - nend))
				logger.fatal(u8"Failed to unmap valid memory");
			return old_addr;
		}

		/* try to incrase the size accordingly */
		if (!env::Instance()->memory().mmap(end, nend - end, usage))
			return errCode::eNoMemory;
		return old_addr;
	}

	/* fetch the new destination address to be used */
	if (detail::IsSet(flags, consts::mmvFixed)) {
		if (!env::Instance()->memory().mmap(new_addr, new_len, usage))
			return errCode::eNoMemory;
	}

	/* try to allocate a new chunk */
	else {
		new_addr = env::Instance()->memory().alloc(new_len, usage);
		if (new_addr == 0)
			return errCode::eNoMemory;
	}

	/* copy the chunk of data over (copy without usage to pretend the underlying memory is moved) */
	uint8_t buffer[4096] = { 0 };
	uint64_t moved = 0, total = std::min<uint64_t>(old_len, new_len);
	while (moved < total) {
		uint64_t count = std::min<uint64_t>(sizeof(buffer), total - moved);
		env::Instance()->memory().mread(buffer, old_addr + moved, count, env::Usage::None);
		env::Instance()->memory().mwrite(new_addr + moved, buffer, count, env::Usage::None);
		moved += count;
	}

	/* release the old allocation or whipe it */
	if (detail::IsSet(flags, consts::mmvDontUnmap))
		env::Instance()->memory().mclear(old_addr, old_len, env::Usage::None);
	else if (!env::Instance()->memory().munmap(old_addr, old_len))
		logger.fatal(u8"Failed to unmap valid memory");
	return new_addr;
}
