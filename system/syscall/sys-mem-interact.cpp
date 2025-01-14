#include "../system.h"

static util::Logger logger{ u8"sys::Syscall" };

bool sys::detail::MemoryInteract::setup(env::guest_t endOfData) {
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
	env::guest_t aligned = ((address + pPageSize - 1) & ~(pPageSize - 1));

	/* check if memory can be released (ignore failure of unmapping the memory) */
	if (aligned < pBrk.aligned) {
		if (!env::Instance()->memory().munmap(aligned, uint32_t(pBrk.aligned - aligned)))
			logger.warn(u8"Unable to release break-memory");
		else
			pBrk.aligned = aligned;
	}

	/* check if memory needs to be allocated */
	else if (aligned > pBrk.aligned) {
		if (!env::Instance()->memory().mmap(pBrk.aligned, uint32_t(aligned - pBrk.aligned), env::Usage::ReadWrite)) {
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
	logger.fatal(u8"Currently not implemented");
	return errCode::eNoMemory;
}
