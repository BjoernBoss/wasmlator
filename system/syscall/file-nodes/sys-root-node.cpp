#include "../../system.h"

sys::detail::impl::RootFileNode::RootFileNode(detail::Syscall* syscall, env::FileAccess access) : NativeFileNode{ syscall, 0 }, pAccess{ access } {
	pUniqueId = util::UniqueId();
}

void sys::detail::impl::RootFileNode::fPatchStats(env::FileStats& stats) const {
	stats.type = env::FileType::directory;
	stats.access = pAccess;
	stats.uniqueId = pUniqueId;
	stats.virtualized = true;
}

void sys::detail::impl::RootFileNode::enable() {
	pEnabled = true;
}

int64_t sys::detail::impl::RootFileNode::stats(std::function<int64_t(const env::FileStats*)> callback) const {
	/* check if the node is not yet enabled, in which case the empty stats need to be provided */
	if (!pEnabled) {
		env::FileStats stats;
		fPatchStats(stats);
		return callback(&stats);
	}

	/* query the actual stats */
	env::Instance()->filesystem().readStats(u8"/", [this, callback](const env::FileStats* stats) {
		pSyscall->callContinue([this, callback, stats]() -> int64_t {
			/* the root must exist */
			if (stats == 0) {
				env::FileStats astats;
				fPatchStats(astats);
				return callback(&astats);
			}

			/* update the stats and return them */
			env::FileStats astats{ *stats };
			fPatchStats(astats);
			return callback(&astats);
			});
		});

	/* defer the call */
	pSyscall->callIncomplete();
	return errCode::eUnknown;
}
