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

	/* iterate over all allocataions within the range */
	while (true) {
		range = env::Instance()->memory().findNext(range.first + range.second);

		/* check if the end has been reached, or if the mapping lies outside of the checked range */
		if (range.second == 0 || range.first >= end)
			break;

		/* check if the mapping can be removed */
		if (!replace)
			return false;

		/* patch the range to unmap all of the memory, which overlaps with the source paramter */
		range.second = std::min<uint64_t>(range.first + range.second, end);
		if (!env::Instance()->memory().munmap(range.first, range.second))
			logger.fatal(u8"Failed to unmap existing range [", str::As{ U"#018x", range.first }, u8"] - [", str::As{ U"#018x", range.first + range.second - 1 });
	}

	/* iterate over the shared ranges and remove all overlapping entries (no need to check th replace-flags, as
	*	the shared mappings are a subset of the mmap-ranges, which would therefore already have thrown an error) */
	auto it = pShared.upper_bound(address);
	if (it != pShared.begin())
		--it;
	while (it != pShared.end()) {
		env::guest_t tempEnd = it->first + it->second.length;

		/* check if the size of the range just needs to be reduced */
		if (it->first < address) {
			if (tempEnd > address) {
				if (!replace)
					logger.fatal(u8"Consistency check between env::Memory and shared-mapping failed");
				it->second.length = address - it->first;
			}
			++it;
			continue;
		}

		/* check if the end has been reached */
		if (it->first >= end)
			break;
		if (!replace)
			logger.fatal(u8"Consistency check between env::Memory and shared-mapping failed");

		/* check if the end of the range can be kept */
		if (tempEnd > end) {
			pShared.erase(it);
			pShared.insert({ end, detail::MemShared{ tempEnd - end } });
			break;
		}

		/* remove the entire overlapped range */
		it = pShared.erase(it);
	}
	return true;
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

		/* check if a any address can be picked */
		if (!detail::IsSet(flags, consts::mmFlagFixed) && !detail::IsSet(flags, consts::mmFlagFixedNoReplace)) {
			address = env::Instance()->memory().alloc(alignedLength, usage);
			if (address == 0)
				return errCode::eNoMemory;
		}

		/* ensure that the address cannot be interpreted as a negative number (otherwise no distinction between errors possible) */
		else if (int64_t(address) < 0)
			return errCode::eNoMemory;

		/* first check if the range is empty or needs to be cleared */
		else if (!fCheckRange(address, alignedLength, !detail::IsSet(flags, consts::mmFlagFixedNoReplace)))
			return errCode::eExists;

		/* allocate the requested range */
		else if (!env::Instance()->memory().mmap(address, alignedLength, usage))
			return errCode::eNoMemory;

		/* update the shared mappings (no chance for collisions, as the shared-mappings are a subset of the memory-mapper) */
		if (shared)
			pShared.insert({ address, detail::MemShared{ length } });
		return address;
	}

	/* validate the validity of the file-descriptor */
	detail::FdState state = pSyscall->files().fdCheck(fd);
	if (!state.valid)
		return errCode::eBadFd;
	if (state.type != env::FileType::file || !state.read)
		return errCode::eAccess;
	if (detail::IsSet(protect, consts::mmProtWrite)) {
		if ((shared && !state.write) || state.append)
			return errCode::eAccess;
	}

	/* fetch the file-states */
	return pSyscall->files().fdStats(fd, [](int64_t result, const env::FileStats* stats) -> int64_t {
		/* check if an error occurred */
		if (result != errCode::eSuccess)
			return result;

		return errCode::eNoMemory;
		});
}
