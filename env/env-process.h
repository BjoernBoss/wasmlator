#pragma once

#include "env-common.h"
#include "context/env-context.h"
#include "mapping/env-mapping.h"
#include "memory/env-memory.h"
#include "process/process-bridge.h"
#include "process/process-access.h"

namespace env {
	env::Process* Instance();

	class Process {
		friend struct detail::ProcessBridge;
		friend struct detail::ProcessAccess;
	private:
		std::function<void()> pLoaded;
		std::vector<env::BlockExport> pExports;
		env::Context pContext;
		env::Memory pMemory;
		env::Mapping pMapping;
		uint32_t pManagementPages = 0;
		uint32_t pPhysicalPages = 0;

	private:
		Process() = default;
		Process(env::Process&&) = delete;
		Process(const env::Process&) = delete;
		~Process() = default;

	public:
		static void Create(uint32_t caches, uint32_t context);

	private:
		void fCoreLoaded(bool succeeded);
		void fBlockLoaded(bool succeeded);

	public:
		void release();

	public:
		void loadCore(const uint8_t* data, size_t size, const std::vector<env::BlockExport>& exports, std::function<void()> callback);
		void loadBlock(const uint8_t* data, size_t size, const std::vector<env::BlockExport>& exports, std::function<void()> callback);

	public:
		const env::Context& context() const;
		env::Context& context();
		const env::Memory& memory() const;
		env::Memory& memory();
		const env::Mapping& mapping() const;
		env::Mapping& mapping();
	};
}
