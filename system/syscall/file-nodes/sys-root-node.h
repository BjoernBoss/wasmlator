#pragma once

#include "sys-node-base.h"
#include "sys-dev-nodes.h"
#include "sys-proc-nodes.h"
#include "sys-native-node.h"

namespace sys::detail::impl {
	/* root file-node, which provides access to all remaining file nodes */
	class RootFileNode final : public impl::NativeFileNode {
	private:
		env::FileAccess pAccess;
		uint64_t pUniqueId = 0;
		bool pEnabled = false;

	public:
		RootFileNode(detail::Syscall* syscall, env::FileAccess access);

	private:
		void fPatchStats(env::FileStats& stats) const;

	public:
		void enable();

	public:
		int64_t stats(std::function<int64_t(const env::FileStats*)> callback) const final;
	};
}
