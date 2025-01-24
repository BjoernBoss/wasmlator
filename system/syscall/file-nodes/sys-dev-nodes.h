#pragma once

#include "sys-node-base.h"

namespace sys::detail::impl {
	class Terminal final : public detail::VirtualFileNode {
	public:
		Terminal(env::FileAccess access);

	public:
		int64_t open(bool truncate, std::function<int64_t(int64_t)> callback) final;
		int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
		int64_t virtualRead(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t virtualWrite(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
	};
}
