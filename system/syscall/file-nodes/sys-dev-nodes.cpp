/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "../../system.h"

static util::Logger logger{ u8"sys::syscall" };

sys::detail::impl::Terminal::Terminal(detail::Syscall* syscall, env::FileAccess access) : VirtFileNode{ env::FileType::terminal, access }, pSyscall{ syscall } {}
int64_t sys::detail::impl::Terminal::open(bool tryRead, bool tryWrite, bool truncate, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eSuccess);
}
int64_t sys::detail::impl::Terminal::read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* ignore any offset - as this is a character-device */

	/* read the data from the input and write them out to the guest buffer */
	env::Instance()->readInput(buffer.size(), [this, callback, &buffer](std::u8string_view read) {
		pSyscall->callContinue([read, callback, &buffer]() -> int64_t {
			std::copy(read.begin(), read.end(), reinterpret_cast<char8_t*>(buffer.data()));
			return callback(read.size());
			});
		});
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::Terminal::write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* ignore any offset - as this is a character-device */
	host::PrintOut({ reinterpret_cast<const char8_t*>(buffer.data()), buffer.size() });
	return callback(buffer.size());
}

sys::detail::impl::URandom::URandom(detail::Syscall* syscall, env::FileAccess access) : VirtFileNode{ env::FileType::character, access }, pSyscall{ syscall } {}
int64_t sys::detail::impl::URandom::open(bool tryRead, bool tryWrite, bool truncate, std::function<int64_t(int64_t)> callback) {
	if (tryWrite)
		return callback(errCode::ePermissionDenied);
	return callback(errCode::eSuccess);
}
int64_t sys::detail::impl::URandom::read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* ignore any offset - as this is a character-device */

	/* fill the buffer with random data */
	for (size_t i = 0; i < buffer.size(); i += 4) {
		uint32_t data = host::GetRandom();
		size_t len = std::min<size_t>(buffer.size() - i, 4);
		std::memcpy(buffer.data() + i, &data, len);
	}
	return callback(buffer.size());
}
int64_t sys::detail::impl::URandom::write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eReadOnly);
}

sys::detail::impl::Null::Null(detail::Syscall* syscall, env::FileAccess access) : VirtFileNode{ env::FileType::character, access }, pSyscall{ syscall } {}
int64_t sys::detail::impl::Null::open(bool tryRead, bool tryWrite, bool truncate, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eSuccess);
}
int64_t sys::detail::impl::Null::read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* overwrite all data with null */
	std::memset(buffer.data(), 0, buffer.size());
	return callback(buffer.size());
}
int64_t sys::detail::impl::Null::write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* ignore any written data */
	return callback(buffer.size());
}
