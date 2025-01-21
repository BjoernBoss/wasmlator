#pragma once

#include "sys-node-base.h"
#include "sys-dev-nodes.h"
#include "sys-proc-nodes.h"

namespace sys::detail::impl {
	/* file-node, which provides access to actual native file-objects */
	class NativeFileNode : public detail::FileNode {
	private:
		uint64_t pOpenId = 0;

	protected:
		detail::Syscall* pSyscall = 0;

	public:
		NativeFileNode(detail::Syscall* syscall, uint64_t openId);

	public:
		int64_t stats(std::function<int64_t(const env::FileStats&)> callback) const;
		int64_t linkRead(std::function<int64_t(bool)> callback) final;
		int64_t lookup(std::u8string_view name, const std::u8string& path, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats&)> callback) final;
		int64_t create(std::u8string_view name, const std::u8string& path, const detail::SetupConfig& config, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback) final;
		int64_t open(const detail::SetupConfig& config, std::function<int64_t(int64_t)> callback) final;
		int64_t read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		void close() final;
	};
}
