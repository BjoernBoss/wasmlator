/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include "../../system.h"

sys::detail::impl::ProcDirectory::ProcDirectory(detail::Syscall* syscall, env::FileAccess access) : VirtFileNode{ env::FileType::directory, access }, pSyscall{ syscall } {}
int64_t sys::detail::impl::ProcDirectory::makeFind(std::u8string_view name, std::function<int64_t(const detail::SharedNode&)> callback) const {
	/* parse the id and create the id-directory */
	uint32_t id = str::ParseNumAll<uint32_t>(name, std::numeric_limits<uint32_t>::max());
	if (id == pSyscall->process().pid)
		return callback(std::make_shared<impl::ProcId>(pSyscall, env::FileAccess{ fs::RootOwner, fs::RootGroup, fs::ReadExecute }, id));
	return callback({});
}
int64_t sys::detail::impl::ProcDirectory::makeListNames(std::function<int64_t(int64_t, const std::vector<std::u8string>&)> callback) {
	return callback(errCode::eSuccess, { str::u8::Int(pSyscall->process().pid) });
}

sys::detail::impl::ProcId::ProcId(detail::Syscall* syscall, env::FileAccess access, uint32_t id) : VirtFileNode{ env::FileType::directory, access }, pSyscall{ syscall }, pId{ id } {}
int64_t sys::detail::impl::ProcId::makeFind(std::u8string_view name, std::function<int64_t(const detail::SharedNode&)> callback) const {
	if (name == u8"exe")
		return callback(std::make_shared<impl::LinkNode>(pSyscall->process().path, env::FileAccess{ fs::DefOwner, fs::DefGroup, fs::All }));
	return callback({});
}
int64_t sys::detail::impl::ProcId::makeListNames(std::function<int64_t(int64_t, const std::vector<std::u8string>&)> callback) {
	return callback(errCode::eSuccess, { u8"exe" });
}
