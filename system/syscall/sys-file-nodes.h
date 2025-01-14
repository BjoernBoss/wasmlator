#pragma once

#include "../sys-common.h"

namespace sys::detail {
	struct NodeStats : public env::FileStats {
		bool isPipe = false;
		bool isTerminal = false;
	};

	/* Note: operations must at most throw syscall-await exception, but always call the callback
	*  Note: object might be deleted within its callback */
	class FileNode {
	private:
		std::u8string pPath;
		size_t pIndex = 0;
		bool pDirectory = false;

	protected:
		FileNode(std::u8string_view path, bool directory);

	public:
		virtual ~FileNode() = default;

	public:
		virtual int64_t setup(std::function<int64_t(int64_t)> callback) = 0;
		virtual int64_t close(std::function<int64_t(int64_t)> callback) = 0;
		virtual int64_t read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) = 0;
		virtual int64_t write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) = 0;
		virtual int64_t stats(std::function<int64_t(int64_t, const detail::NodeStats&)> callback) = 0;
		virtual void modify(bool blocking) = 0;

	public:
		const std::u8string& path() const;
		bool directory() const;
		void bind(size_t index);
		size_t index() const;
	};

	namespace impl {
		class Terminal final : public detail::FileNode {
		private:
			uint64_t pLastRead = 0;
			uint64_t pLastWrite = 0;
			bool pBlocking = false;

		public:
			Terminal();

		public:
			int64_t setup(std::function<int64_t(int64_t)> callback) final;
			int64_t close(std::function<int64_t(int64_t)> callback) final;
			int64_t read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
			int64_t write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
			int64_t stats(std::function<int64_t(int64_t, const detail::NodeStats&)> callback) final;
			void modify(bool blocking) final;
		};
	}
}
