#pragma once

#include <wasgen/wasm.h>
#include <inttypes.h>

namespace glue {
	static constexpr uint32_t PageSize = 0x10000;

	enum class Mapping : uint8_t {
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

	/* any state greater than 'coreLoaded' assumes the core to be loaded */
	enum class SlotState : uint8_t {
		reserved = 0,
		available = 1,
		awaitingCore = 2,
		loadingCore = 3,
		coreLoaded = 4
	};

	struct String {
		uint32_t offset = 0;
		uint32_t length = 0;
	};

	struct State {
	public:
		wasm::Module module;
		wasm::Function hostLoadCore;
		wasm::Function hostGetFunction;
		wasm::Memory memory;
		wasm::Table functions;
		wasm::Table cores;
		wasm::Global slotCount;
		glue::String strings[size_t(glue::Mapping::_count)];
		uint32_t addressOfList = 0;

	public:
		State(wasm::ModuleInterface* writer) : module{ writer } {}
	};

	/*
	*	Initialize the state and write the strings to memory (after the imports have been added)
	*/
	void InitializeState(glue::State& state);

	/*
	*	Define the imports the environment needs to supply to the glue-module
	*
	*	Called by glue-code to trigger instantiation of the core wasm-module
	*		void host.host_load_core(uint32_t id, uint32_t ptr, uint32_t size);
	*
	*	Called by glue-code to get an exported function reference with the given name from the object
	*		func_ref host.host_get_function(extern_ref object, uint32_t ptr, uint32_t size);
	*/
	void SetupHostImports(glue::State& state);

	/*
	*	Define the functions the host environment interacts with
	*
	*	Called by host-environment with core-reference object (the loaded wasm-module) and the id it belongs to
	*		void host_core_callback(uint32_t id, extern_ref core);
	*/
	void SetupHostBody(glue::State& state);

	/*
	*	Define the functions the main application context interacts with
	*
	*	Allocate a new context id (returns 0 on failure, else 1)
	*		uint32_t ctx_create();
	*
	*	Setup the core-module of the newly created context (returns 0 if core is already set and otherwise 1)
	*		uint32_t ctx_set_core(uint32_t id, uint32_t ptr, uint32_t size);
	*
	*	Destroy the created context and release any belonging resources
	*		void ctx_destroy(uint32_t id);
	*
	*	Check if the core has been loaded (return 0 if not yet completed loading, else 1)
	*		uint32_t ctx_core_loaded(uint32_t id);
	*/
	void SetupContextFunctions(glue::State& state);

	/*
	*	Implement the separate memory interaction functions, which are
	*	directly forwarded to the functions exported from the core module
	*/
	void SetupMemoryFunctions(glue::State& state);
}
