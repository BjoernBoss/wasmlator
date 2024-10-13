#pragma once

#include <wasgen/wasm.h>
#include <cinttypes>

namespace glue {
	static constexpr uint32_t PageSize = 0x10000;
	static constexpr uint32_t MinCoreCount = 2;

	enum class CoreMapping : uint8_t {
		execute,
		blocksReserve,
		blocksLoaded,
		blocksGetLast,
		blocksAddExport,
		flushBlocks,

		expandPhysical,
		movePhysical,
		flushCaches,

		readi32Fromu8,
		readi32Fromi8,
		readi32Fromu16,
		readi32Fromi16,
		readi32,
		readi64,
		readf32,
		readf64,

		writei32Fromu8,
		writei32Fromi8,
		writei32Fromu16,
		writei32Fromi16,
		writei32,
		writei64,
		writef32,
		writef64,

		executei32Fromu8,
		executei32Fromi8,
		executei32Fromu16,
		executei32Fromi16,
		executei32,
		executei64,
		executef32,
		executef64,

		_count
	};

	enum class MainMapping : uint8_t {
		_initialize,
		startup,
		coreLoaded,
		blockLoaded,

		_count
	};

	enum class MainState :uint8_t {
		notLoaded,
		loaded,
		failed
	};

	/* any state greater than 'coreLoaded' assumes the core to be loaded */
	enum class SlotState : uint8_t {
		reserved = 0,
		available = 1,
		awaitingCore = 2,
		loadingCore = 3,
		coreFailed = 4,
		coreLoaded = 5,
		loadingBlock = 6
	};
	struct Slot {
		uint64_t process = 0;
		glue::SlotState state = SlotState::reserved;
	};

	struct String {
		uint32_t offset = 0;
		uint32_t length = 0;
	};

	struct State {
	public:
		wasm::Module module;
		wasm::Function hostLoadCore;
		wasm::Function hostLoadBlock;
		wasm::Function hostGetMainFunction;
		wasm::Function hostGetCoreFunction;
		wasm::Function hostGetBlockFunction;
		wasm::Memory memory;
		wasm::Table mainFunctions;
		wasm::Table coreFunctions;
		wasm::Table cores;
		wasm::Global mainLoaded;
		wasm::Global slotCount;
		glue::String mainStrings[size_t(glue::MainMapping::_count)];
		glue::String coreStrings[size_t(glue::CoreMapping::_count)];
		uint32_t addressOfList = 0;

	public:
		State(wasm::ModuleInterface* writer) : module{ writer } {}
	};

	/*
	*	Perform the actual generation
	*/
	bool Generate(const std::string& wasmPath, const std::string& watPath);

	/*
	*	Initialize the state and write the strings to memory (after the imports have been added)
	*
	*	Internally reserve slots for exports and a new block
	*		uint32_t reserve_block(uint32_t id, uint32_t exports);
	*/
	void InitializeState(glue::State& state);

	/*
	*	Define the imports the environment needs to supply to the glue-module
	*
	*	Called by glue-code to trigger instantiation of the core wasm-module
	*		void host.host_load_core(uint32_t id, uint32_t ptr, uint32_t size);
	*
	*	Called by glue-code to get an exported function reference from the main instance
	*		func_ref host.host_get_main_export(uint32_t ptr, uint32_t size);
	*
	*	Called by glue-code to get an exported function reference with the given name from the instance
	*		func_ref host.host_get_core_export(extern_ref instance, uint32_t ptr, uint32_t size);
	*/
	void SetupHostImports(glue::State& state);

	/*
	*	Define the functions the host environment interacts with
	*
	*	Called by host-environment when then main application has been loaded
	*		void host_main_loaded();
	*
	*	Called by host-environment with core-reference object (the loaded wasm-module) and the id it belongs to
	*		void host_core_loaded(uint32_t id, extern_ref instance);
	*/
	void SetupHostBody(glue::State& state);

	/*
	*	Define the functions the main application context interacts with
	*
	*	Allocate a new context id (returns 0 on failure, else 1)
	*		uint32_t ctx_create(uint64_t process);
	*
	*	Setup the core-module of the newly created context (returns 0 if core is already set and otherwise 1)
	*		uint32_t ctx_load_core(uint32_t id, uint32_t ptr, uint32_t size);
	*
	*	Setup a block-module of a valid context (returns 0 if invalid state or not enough resources and otherwise 1)
	*		uint32_t ctx_load_block(uint32_t id, uint32_t ptr, uint32_t size, uint32_t exports);
	*
	*	Destroy the created context and release any belonging resources
	*		void ctx_destroy(uint32_t id);
	*/
	void SetupContextFunctions(glue::State& state);

	/*
	*	Implement the separate blocks interaction functions, which are
	*	directly forwarded to the functions exported from the core module
	*/
	void SetupBlocksFunctions(glue::State& state);

	/*
	*	Implement the separate memory interaction functions, which are
	*	directly forwarded to the functions exported from the core module
	*/
	void SetupMemoryFunctions(glue::State& state);
}
