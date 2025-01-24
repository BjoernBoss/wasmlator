#pragma once

#include "sys-node-base.h"
#include "sys-dev-nodes.h"
#include "sys-proc-nodes.h"
#include "sys-native-node.h"

namespace sys::detail::impl {
	/* root file-node, which provides access to all remaining file nodes */
	class RootFileNode final : public detail::FileNode {
	private:
		mutable detail::SharedNode pNative;
		env::FileAccess pAccess;
		detail::Syscall* pSyscall = 0;
		uint64_t pUniqueId = 0;
		bool pEnabled = false;

	public:
		RootFileNode(detail::Syscall* syscall, env::FileAccess access);

	private:
		void fPatchStats(env::FileStats& stats) const;
		int64_t fNullStats(std::function<int64_t(const env::FileStats*)> callback) const;
		int64_t fWithNative(std::function<int64_t()> callback) const;

	public:
		void enable();

	public:
		int64_t stats(std::function<int64_t(const env::FileStats*)> callback) const final;
		int64_t lookup(std::u8string_view name, const std::u8string& path, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats&)> callback) final;
		int64_t create(std::u8string_view name, const std::u8string& path, env::FileAccess access, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback) final;
	};
}
