#pragma once

#include "env-common.h"
#include "context/env-context.h"
#include "mapping/env-mapping.h"
#include "memory/env-memory.h"
#include "interact/env-interact.h"
#include "process/process-bridge.h"
#include "process/process-access.h"
#include "../system/sys-specification.h"

namespace env {
	env::Process* Instance();

	class Process {
		friend struct detail::ProcessBridge;
		friend struct detail::ProcessAccess;
	private:
		std::unique_ptr<sys::Specification> pSpecification;
		std::function<void()> pLoaded;
		env::Context pContext;
		env::Memory pMemory;
		env::Mapping pMapping;
		env::Interact pInteract;
		uint32_t pManagementPages = 0;
		uint32_t pPhysicalPages = 0;

	private:
		Process(std::unique_ptr<sys::Specification>&& specification);
		Process(env::Process&&) = delete;
		Process(const env::Process&) = delete;
		~Process() = default;

	public:
		static void Create(std::unique_ptr<sys::Specification>&& specification);

	private:
		void fLoadCore();
		void fCoreLoaded(bool succeeded);

	public:
		void release();

	public:
		const sys::Specification& specification() const;
		sys::Specification& specification();
		const env::Context& context() const;
		env::Context& context();
		const env::Memory& memory() const;
		env::Memory& memory();
		const env::Mapping& mapping() const;
		env::Mapping& mapping();
		const env::Interact& interact() const;
		env::Interact& interact();
	};
}
