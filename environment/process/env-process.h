#pragma once

#include "../env-common.h"
#include "process-bridge.h"
#include "process-access.h"
#include "../context/env-context.h"
#include "../mapping/env-mapping.h"
#include "../filesystem/env-filesystem.h"
#include "../memory/env-memory.h"
#include "../interact/env-interact.h"

namespace env {
	class Process {
		friend struct detail::ProcessBridge;
		friend struct detail::ProcessAccess;
	private:
		enum class ProcState : uint8_t {
			none,
			loadingCore,
			coreLoaded,
			loadingBlock
		};
		enum class TaskState : uint8_t {
			none,
			started,
			awaiting
		};
		struct Binding {
			std::u8string name;
			uint32_t index = 0;
		};

	private:
		std::unique_ptr<env::System> pSystem;
		std::vector<env::BlockExport> pExports;
		std::unordered_map<std::u8string, std::vector<Binding>> pBindings;
		std::function<void(std::u8string_view, bool)> pTaskCallback;
		env::Context pContext;
		env::Memory pMemory;
		env::FileSystem pFileSystem;
		env::Mapping pMapping;
		env::Interact pInteract;
		std::u8string pBlockImportName;
		size_t pTaskStamp = 0;
		uint32_t pPageSize = 0;
		uint32_t pMemoryCaches = 0;
		uint32_t pContextSize = 0;
		uint64_t pPhysicalPages = 0;
		uint64_t pMemoryPages = 0;
		ProcState pProcState = ProcState::none;
		TaskState pTaskState = TaskState::none;
		bool pBindingsClosed = false;
		bool pLogBlocks = false;
		bool pDetectWriteExecute = false;

	public:
		Process() = default;
		Process(env::Process&&) = delete;
		Process(const env::Process&) = delete;
		~Process() = default;

	private:
		bool fSetup(std::unique_ptr<env::System>&& system, uint32_t pageSize, uint32_t memoryCaches, uint32_t contextSize, bool detectWriteExecute, bool logBlocks);
		void fAddBinding(const std::u8string& mod, const std::u8string& name);
		bool fHandleTask(const std::u8string& task, std::function<void(std::u8string_view, bool)> callback);
		bool fTaskCompleted(uint32_t process, std::u8string_view response);

	private:
		bool fLoadCore();
		void fCoreLoaded();
		void fLoadBlock();
		void fBlockLoaded();

	public:
		const std::u8string& blockImportModule() const;
		void bindExport(std::u8string_view name);
		void startNewBlock();

	public:
		void shutdown();
		const env::Context& context() const;
		env::Context& context();
		const env::FileSystem& filesystem() const;
		env::FileSystem& filesystem();
		const env::Memory& memory() const;
		env::Memory& memory();
		const env::Mapping& mapping() const;
		env::Mapping& mapping();
		const env::Interact& interact() const;
		env::Interact& interact();

	public:
		uint32_t pageSize() const;
		uint32_t memoryCaches() const;
		uint32_t contextSize() const;
		bool logBlocks() const;
		bool detectWriteExecute() const;

	public:
		bool readInput(size_t max, std::function<void(std::u8string_view)> callback);
	};
}
