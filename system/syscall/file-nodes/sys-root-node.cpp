/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "../../system.h"

static util::Logger logger{ u8"sys::syscall" };

sys::detail::impl::RootFileNode::RootFileNode(detail::Syscall* syscall, env::FileAccess access) : FileNode{ util::UniqueId(), true, env::FileType::directory }, pSyscall{ syscall }, pAccess{ access } {}

int64_t sys::detail::impl::RootFileNode::fWithNative(std::function<int64_t()> callback) const {
	if (pNative.get() != 0)
		return callback();

	/* try to create the native node */
	env::Instance()->filesystem().readStats(u8"/", [this, callback](const env::FileStats* stats) {
		pSyscall->callContinue([this, callback, stats]() -> int64_t {
			if (stats == 0) {
				logger.fatal(u8"Failed to fetch stats of root-node");

				/* should never be reached */
				return errCode::eUnknown;
			}
			pNative = std::make_shared<impl::NativeFileNode>(stats->type, pSyscall, stats->id);
			return callback();
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}

void sys::detail::impl::RootFileNode::enable() {
	pEnabled = true;
}

int64_t sys::detail::impl::RootFileNode::stats(std::function<int64_t(const detail::NodeStats&)> callback) const {
	/* check if the node is not yet enabled, in which case the empty stats need to be provided */
	if (!pEnabled)
		return callback(detail::NodeStats{ .access = pAccess });

	/* perform the stats-lookup */
	return fWithNative([this, callback]() -> int64_t {
		return pNative->stats([this, callback](const detail::NodeStats& stats) -> int64_t {
			detail::NodeStats _stats{ stats };
			_stats.access = pAccess;
			return callback(_stats);
			});
		});
}
int64_t sys::detail::impl::RootFileNode::makeLookup(std::u8string_view name, std::function<int64_t(const detail::SharedNode&, const detail::NodeStats&)> callback) const {
	if (!pEnabled)
		return callback({}, {});
	return fWithNative([this, name = std::u8string{ name }, callback]() -> int64_t {
		return pNative->makeLookup(name, callback);
		});
}
int64_t sys::detail::impl::RootFileNode::makeCreate(std::u8string_view name, env::FileAccess access, std::function<int64_t(int64_t, const detail::SharedNode&)> callback) {
	if (!pEnabled)
		return callback(errCode::eReadOnly, {});
	return fWithNative([this, name = std::u8string{ name }, access, callback]() -> int64_t {
		return pNative->makeCreate(name, access, callback);
		});
}
int64_t sys::detail::impl::RootFileNode::makeListDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback) {
	if (!pEnabled)
		return callback(errCode::eSuccess, {});
	return fWithNative([this, callback]() -> int64_t {
		return pNative->makeListDir(callback);
		});
}
int64_t sys::detail::impl::RootFileNode::flagRead(std::function<int64_t()> callback) {
	if (!pEnabled)
		return callback();
	return fWithNative([this, callback]() -> int64_t {
		return pNative->flagRead(callback);
		});
}
int64_t sys::detail::impl::RootFileNode::flagWritten(std::function<int64_t()> callback) {
	if (!pEnabled)
		return callback();
	return fWithNative([this, callback]() -> int64_t {
		return pNative->flagWritten(callback);
		});
}
