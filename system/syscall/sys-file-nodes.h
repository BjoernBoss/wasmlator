#pragma once

#include "../sys-common.h"

namespace sys::detail {
	/* Note: operations must at most throw syscall-await exception, but always call the callback
	*  Note: object might be deleted within its callback */
	class FileNode {
	private:
		std::u8string pDirectory;
		bool pWritable = false;
		size_t pIndex = 0;

	protected:
		FileNode(bool writable);

	public:
		virtual ~FileNode() = default;

	public:
		virtual int64_t setup(std::function<int64_t(int64_t, const env::FileStats&)> callback) = 0;
		virtual int64_t close(std::function<int64_t()> callback) = 0;
		virtual int64_t read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) = 0;
		virtual int64_t write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) = 0;
		virtual int64_t stats(std::function<int64_t(const env::FileStats&)> callback) = 0;
		virtual void modify(bool blocking) = 0;

	public:
		void bindConfigure(size_t index, const std::u8string_view directory);
		const std::u8string& directory() const;
		bool writable() const;
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

		private:
			env::FileStats fStats() const;

		public:
			int64_t setup(std::function<int64_t(int64_t, const env::FileStats&)> callback) final;
			int64_t close(std::function<int64_t()> callback) final;
			int64_t read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
			int64_t write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
			int64_t stats(std::function<int64_t(const env::FileStats&)> callback) final;
			void modify(bool blocking) final;
		};

		class ProcDirectory final : public detail::FileNode {
		private:
			enum class Value :uint8_t {
				noEntry,
				notDirectory,
				root,
				self,
				selfExe
			};

		private:
			detail::Syscall* pSyscall = 0;
			Value pValue = Value::noEntry;
			uint64_t pLastRead = 0;

		public:
			ProcDirectory(detail::Syscall* syscall, std::u8string_view path);

		private:
			void fMatchType(std::u8string_view path);
			env::FileStats fStats() const;

		public:
			int64_t setup(std::function<int64_t(int64_t, const env::FileStats&)> callback) final;
			int64_t close(std::function<int64_t()> callback) final;
			int64_t read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
			int64_t write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
			int64_t stats(std::function<int64_t(const env::FileStats&)> callback) final;
			void modify(bool blocking) final;
		};
	}
}
