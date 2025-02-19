#include "../../system.h"

sys::detail::impl::RootFileNode::RootFileNode(const detail::SharedNode& ancestor, detail::Syscall* syscall, env::FileAccess access) : FileNode{ ancestor }, pSyscall{ syscall }, pAccess{ access } {
	pUniqueId = util::UniqueId();
}

void sys::detail::impl::RootFileNode::fPatchStats(env::FileStats& stats) const {
	stats.type = env::FileType::directory;
	stats.access = pAccess;
	stats.id = pUniqueId;
	stats.virtualized = true;
}
int64_t sys::detail::impl::RootFileNode::fNullStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	fPatchStats(stats);
	return callback(&stats);
}
int64_t sys::detail::impl::RootFileNode::fWithNative(std::function<int64_t()> callback) const {
	if (pNative.get() != 0)
		return callback();

	/* try to create the native node */
	env::Instance()->filesystem().readStats(u8"/", [this, callback](const env::FileStats* stats) {
		pSyscall->callContinue([this, callback, stats]() -> int64_t {
			if (stats != 0)
				pNative = std::make_shared<impl::NativeFileNode>(FileNode::ancestor(), pSyscall, stats->id);
			return callback();
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}

void sys::detail::impl::RootFileNode::enable() {
	pEnabled = true;
}

int64_t sys::detail::impl::RootFileNode::stats(std::function<int64_t(const env::FileStats*)> callback) const {
	/* check if the node is not yet enabled, in which case the empty stats need to be provided */
	if (!pEnabled)
		return fNullStats(callback);

	/* perform the stats-lookup */
	return fWithNative([this, callback]() -> int64_t {
		/* the root must exist */
		if (pNative.get() == 0)
			return fNullStats(callback);

		/* perform the actual stat-read (the root must exist at all times) */
		return pNative->stats([this, callback](const env::FileStats* stats) -> int64_t {
			if (stats == 0)
				return fNullStats(callback);
			env::FileStats _stats{ *stats };
			fPatchStats(_stats);
			return callback(&_stats);
			});
		});
}
int64_t sys::detail::impl::RootFileNode::lookup(std::u8string_view name, const std::u8string& path, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats&)> callback) {
	if (!pEnabled)
		return callback({}, {});

	/* perform the actual call */
	return fWithNative([this, name = std::u8string{ name }, path, callback]() -> int64_t {
		if (pNative.get() == 0)
			return callback({}, {});
		return pNative->lookup(name, path, callback);
		});
}
int64_t sys::detail::impl::RootFileNode::create(std::u8string_view name, const std::u8string& path, env::FileAccess access, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback) {
	if (!pEnabled)
		return callback({}, {});

	/* perform the actual call */
	return fWithNative([this, name = std::u8string{ name }, path, access, callback]() -> int64_t {
		if (pNative.get() == 0)
			return callback({}, {});
		return pNative->create(name, path, access, callback);
		});
}
