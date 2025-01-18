#pragma once

#include "sys-file-nodes.h"

namespace sys::detail::impl {
	class DevDirectory final : public detail::VirtualFileNode {
	public:
		DevDirectory();

	public:
		std::shared_ptr<detail::FileNode> spawn(const std::u8string& path, std::u8string_view name) final;
		int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
	};

	class DevTerminal final : public detail::VirtualFileNode {
	public:
		DevTerminal();

	public:
		int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
		int64_t virtualRead(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t virtualWrite(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
	};
}
