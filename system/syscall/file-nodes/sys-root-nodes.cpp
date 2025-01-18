#include "../../system.h"

static util::Logger logger{ u8"sys::Syscall" };

sys::detail::impl::RootFileNode::RootFileNode(detail::Syscall* syscall) : FileNode{ u8"/" }, pSyscall{ syscall } {}
int64_t sys::detail::impl::RootFileNode::stats(std::function<int64_t(const env::FileStats*)> callback) const {
	/* perform the native read of the root-stats */
	env::Instance()->filesystem().readStats(u8"/", [=, this](const env::FileStats* stats) {
		if (stats == 0)
			logger.fatal(u8"Failed to fetch stats for [/] directory");
		pSyscall->callContinue([=, this]() -> int64_t { return callback(stats); });
		});

	/* mark the syscall as incomplete (will never reach the return statement) */
	pSyscall->callIncomplete();
	return errCode::eUnknown;
}
std::shared_ptr<sys::detail::FileNode> sys::detail::impl::RootFileNode::spawn(const std::u8string& path, std::u8string_view name) {
	if (name == u8"dev")
		return std::make_shared<impl::DevDirectory>();
	if (name == u8"proc")
		return std::make_shared<impl::ProcDirectory>(pSyscall);
	return std::make_shared<impl::NullFileNode>(path);
}
