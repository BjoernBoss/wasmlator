let _state = {
	main: {
		path: './wasm/main.wasm',
		wasm: null,
		memory: null,
		exports: {}
	},
	glue: {
		path: './wasm/glue-module.wasm',
		wasm: null,
		memory: null,
		exports: {}
	},
	core_exports: null
};

/* execute the callback in a new execution-context where controlled-aborts will not be considered uncaught
*	exceptions, but all other exceptions will, and exceptions do not trigger other catch-handlers */
class _ControlledAbort { }
_state.controlled = function (fn) {
	try {
		fn();
	} catch (e) {
		if (!(e instanceof _ControlledAbort))
			console.error(`Uncaught controlled exception: ${e.stack}`);
	}
}
_state.abortControlled = function () {
	throw new _ControlledAbort();
}

/* create a string from the utf-8 encoded data at ptr in the main application */
_state.load_string = function (ptr, size) {
	let view = new DataView(_state.main.memory.buffer, ptr, size);
	return new TextDecoder('utf-8').decode(view);
}

/* create a buffer (newly allocated) from the data at ptr in the main application */
_state.make_buffer = function (ptr, size) {
	return _state.main.memory.buffer.slice(ptr, ptr + size);
}

/* load the initial glue module and afterwards the main application */
_state.load_glue = function () {
	console.log(`WasmLator.js: Loading glue module...`);

	let load_string = function (ptr, size) {
		let view = new DataView(_state.glue.memory.buffer, ptr, size);
		return new TextDecoder('utf-8').decode(view);
	};

	/* setup the glue imports */
	let imports = {
		host: {}
	};
	imports.host.host_get_export = function (ptr, size) {
		let fn = _state.core_exports[load_string(ptr, size)];
		if (fn === undefined)
			return null;
		return fn;
	};

	/* fetch the initial glue module and try to instantiate it */
	fetch(_state.glue.path, { credentials: 'same-origin' })
		.then((resp) => {
			if (!resp.ok)
				throw new Error(`Failed to load [${_state.glue.path}]`);
			return WebAssembly.instantiateStreaming(resp, imports);
		})
		.then((instance) => {
			console.log(`WasmLator.js: Glue module loaded`);
			_state.glue.wasm = instance.instance;
			_state.glue.exports = _state.glue.wasm.exports;
			_state.glue.memory = _state.glue.exports.memory;

			/* load the main module, which will then startup the wasmlator */
			_state.controlled(() => _state.load_main());
		})
		.catch((err) => {
			console.error(`Failed to load glue module: ${err}`);
		});
}

/* load the actual primary application once the glue module has been loaded and compiled */
_state.load_main = function () {
	console.log(`WasmLator.js: Loading main application...`);

	/* setup the main imports (env.emscripten_notify_memory_growth and wasi_snapshot_preview1.proc_exit required by wasm-standalone module) */
	let imports = {
		env: {},
		wasi_snapshot_preview1: {}
	};
	imports.env.emscripten_notify_memory_growth = function () { };
	imports.wasi_snapshot_preview1.proc_exit = function (code) {
		console.error(`WasmLator.js: Main application unexpectedly terminated itself with [${code}]`);
		_state.abortControlled();
	};
	imports.env.host_print_u8 = function (ptr, size) {
		console.log(_state.load_string(ptr, size));
	};
	imports.env.host_abort = function () {
		_state.abortControlled();
	};
	imports.env.host_load_core = function (ptr, size) {
		_state.load_core(_state.make_buffer(ptr, size));
	};
	imports.env.proc_setup_core_functions = _state.glue.exports.proc_setup_core_functions;
	imports.env.ctx_read = _state.glue.exports.ctx_read;
	imports.env.ctx_write = _state.glue.exports.ctx_write;
	imports.env.map_load_block = _state.glue.exports.map_load_block;
	imports.env.map_define = _state.glue.exports.map_define;
	imports.env.map_flush_blocks = _state.glue.exports.map_flush_blocks;
	imports.env.map_execute = _state.glue.exports.map_execute;
	imports.env.mem_flush_caches = _state.glue.exports.mem_flush_caches;
	imports.env.mem_expand_physical = _state.glue.exports.mem_expand_physical;
	imports.env.mem_move_physical = _state.glue.exports.mem_move_physical;
	imports.env.mem_read = _state.glue.exports.mem_read;
	imports.env.mem_write = _state.glue.exports.mem_write;
	imports.env.mem_code = _state.glue.exports.mem_code;

	/* fetch the main application javascript-wrapper */
	fetch(_state.main.path, { credentials: 'same-origin' })
		.then((resp) => {
			if (!resp.ok)
				throw new Error(`Failed to load [${_state.main.path}]`);
			return WebAssembly.instantiateStreaming(resp, imports);
		})
		.then((instance) => {
			console.log(`WasmLator.js: Main module loaded`);
			_state.main.wasm = instance.instance;
			_state.main.exports = _state.main.wasm.exports;
			_state.main.memory = _state.main.exports.memory;

			/* startup the main application, which requires the internal _initialize and explicitly created main_startup
			*	to be invoked (do this in a separate call to prevent exceptions from propagating into the catch-handler) */
			_state.controlled(() => {
				console.log(`WasmLator.js: Starting up main module...`);
				_state.main.exports._initialize();
				_state.main.exports.main_startup();
			});
		})
		.catch((err) => {
			_state.controlled(() => {
				console.error(`WasmLator.js: Failed to load main module: ${err}`);
				_state.glue.exports.host_main_loaded(0);
			});
		});
}

/* load a core module */
_state.load_core = function (buffer) {
	console.log(`WasmLator.js: Loading core...`);

	/* setup the core imports */
	let imports = {
		main: {},
		host: {}
	};
	imports.main.main_set_exit_code = _state.main.exports.main_set_exit_code;
	imports.main.main_resolve = _state.main.exports.main_resolve;
	imports.main.main_flushed = _state.main.exports.main_flushed;
	imports.main.main_block_loaded = _state.main.exports.main_block_loaded;
	imports.main.main_lookup = _state.main.exports.main_lookup;
	imports.main.main_result_address = _state.main.exports.main_result_address;
	imports.main.main_result_physical = _state.main.exports.main_result_physical;
	imports.main.main_result_size = _state.main.exports.main_result_size;
	imports.host.host_load_block = function(ptr, size) {
		_state.load_core(_state.make_buffer(ptr, size));
	}
	imports.host.host_get_export = function(instance, ptr, size) {
		let fn = instance.exports[_state.load_string(ptr, size)];
		if (fn === undefined)
			return null;
		return fn;
	}

	/* try to instantiate the core module */
	WebAssembly.instantiate(buffer, imports)
		.then((instance) => {
			/* notify the main about the loaded core(do this in a separate
			*	call to prevent exceptions from propagating into the catch-handler) */
			_state.controlled(() => {
				console.log(`WasmLator.js: Core loaded`);
				_state.core_exports = instance.instance.exports;
				_state.main.exports.main_core_loaded(1);
			});
		})
		.catch((err) => {
			_state.controlled(() => {
				console.error(`WasmLator.js: Failed to load core: ${err}`);
				_state.main.exports.main_core_loaded(0);
			});
		});
}

/* load a block module */
_state.load_block = function (core, id, buffer) {
	console.log(`WasmLator.js: Instantiating block for [${id}]...`);

	/* setup the block imports */
	let imports = {
		core: {},
		mem: {},
		ctx: {},
		map: {}
	};
	imports.core.memory_physical = core.exports.memory_physical;
	imports.core.memory_management = core.exports.memory_management;
	imports.mem.lookup_read = core.exports.mem_lookup_read;
	imports.mem.lookup_write = core.exports.mem_lookup_write;
	imports.mem.lookup_code = core.exports.mem_lookup_code;
	imports.map.goto = core.exports.map_goto;
	imports.map.lookup = core.exports.map_lookup;
	imports.ctx.exit = core.exports.ctx_exit;

	/* try to instantiate the block module */
	WebAssembly.instantiate(buffer, imports)
		.then((instance) => {
			/* notify the glue module about the loaded block (do this in a separate
			*	call to prevent exceptions from propagating into the catch-handler) */
			_state.controlled(() => {
				console.log(`WasmLator.js: Block for [${id}] loaded`);
				_state.glue.exports.host_block_loaded(id, instance.instance);
			});
		})
		.catch((err) => {
			_state.controlled(() => {
				console.error(`WasmLator.js: Failed to load block for [${id}]: ${err}`);
				_state.glue.exports.host_block_loaded(id, null);
			});
		});
}

_state.load_glue();
