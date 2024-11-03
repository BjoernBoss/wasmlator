#pragma once

#include "env-common.h"
#include "mapping/env-mapping.h"
#include "memory/env-memory.h"
#include "process/process-bridge.h"
#include "process/process-access.h"

namespace env {
	class Process {
		friend struct bridge::Process;
		friend class detail::ProcessAccess;
	private:
		std::function<void(bool)> pLoaded;
		std::vector<env::BlockExport> pExports;
		env::Memory pMemory;
		env::Mapping pMapping;
		uint32_t pManagementPages = 0;
		uint32_t pPhysicalPages = 0;
		std::u8string pName;
		std::u8string pLogHeader;
		env::id_t pId = 0;

	private:
		Process(std::u8string_view name);
		Process(env::Process&&) = delete;
		Process(const env::Process&) = delete;
		~Process() = default;

	public:
		static env::Process* Create(std::u8string_view name, uint32_t caches);

	private:
		template <class... Args>
		void fLog(const Args&... args) const {
			util::log(u8"Process ", pLogHeader, args...);
		}
		template <class... Args>
		void fDebug(const Args&... args) const {
			util::log(u8"Process ", pLogHeader, args...);
		}
		template <class... Args>
		void fFail(const Args&... args) const {
			util::fail(u8"Failure ", pLogHeader, args...);
		}

	private:
		void fCoreLoaded(bool succeeded);
		void fBlockLoaded(bool succeeded);

	public:
		void release();

	public:
		void loadCore(const uint8_t* data, size_t size, std::function<void(bool)> callback);
		void loadBlock(const uint8_t* data, size_t size, const std::vector<env::BlockExport>& exports, std::function<void(bool)> callback);
		const std::u8string& name() const;
		env::id_t id() const;

	public:
		const env::Memory& memory() const;
		env::Memory& memory();
		const env::Mapping& mapping() const;
		env::Mapping& mapping();

	public:
		template <class... Args>
		void log(const Args&... args) const {
			fLog(args...);
		}
		template <class... Args>
		void debug(const Args&... args) const {
			fDebug(args...);
		}
		template <class... Args>
		void fail(const Args&... args) const {
			fFail(args...);
		}
	};
}
