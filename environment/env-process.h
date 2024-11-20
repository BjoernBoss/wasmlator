#pragma once

#include "env-common.h"
#include "context/env-context.h"
#include "mapping/env-mapping.h"
#include "memory/env-memory.h"
#include "interact/env-interact.h"
#include "process/process-bridge.h"
#include "process/process-access.h"

namespace env {
	/* Many operations on a process must first be called after the core has been loaded, as they might internally
	*		interact with the core, which will otherwise result in a null-function execution within the glue-module
	*	Note: must only be destroyed form within coreLoaded/blockLoaded or the main startup */
	env::Process* Instance();

	class Process {
		friend struct detail::ProcessBridge;
		friend struct detail::ProcessAccess;
	private:
		enum class State : uint8_t {
			none,
			loadingCore,
			coreLoaded,
			loadingBlock
		};
		struct Binding {
			std::u8string name;
			uint32_t index = 0;
		};

	private:
		std::unique_ptr<env::System> pSystem;
		std::vector<env::BlockExport> pExports;
		std::unordered_map<std::u8string, std::vector<Binding>> pBindings;
		env::Context pContext;
		env::Memory pMemory;
		env::Mapping pMapping;
		env::Interact pInteract;
		uint32_t pPhysicalPages = 0;
		uint32_t pMemoryPages = 0;
		State pState = State::none;

	private:
		Process(std::unique_ptr<env::System>&& system);
		Process(env::Process&&) = delete;
		Process(const env::Process&) = delete;
		~Process() = default;

	public:
		static void Create(std::unique_ptr<env::System>&& system);

	private:
		void fLoadCore();
		void fLoadBlock();
		bool fCoreLoaded(uint32_t process, bool succeeded);
		bool fBlockLoaded(uint32_t process, bool succeeded);
		void fAddBinding(const std::u8string& mod, const std::u8string& name);

	public:
		void startNewBlock();
		void release();

	public:
		const env::System& system() const;
		env::System& system();
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
