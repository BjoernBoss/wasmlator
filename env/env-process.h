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

	public:
		void loadCore(const uint8_t* data, size_t size, std::function<void()> callback);
		void release();

	public:
		const env::Context& context() const;
		env::Context& context();
		const env::Memory& memory() const;
		env::Memory& memory();
		const env::Mapping& mapping() const;
		env::Mapping& mapping();
	};
}
