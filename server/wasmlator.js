let _state = {
	main: {
		path: './main.wasm',
		wasm: null,
		memory: null,
		exports: {}
	},
	glue: {
		path: './generated/glue-module.wasm',
		wasm: null,
		memory: null,
		exports: {}
	}
};

/* create a string from the utf-8 encoded data at ptr in the glue application */
_state.load_glue_string = function (ptr, size) {
	let view = new DataView(_state.glue.memory.buffer, ptr, size);
	return new TextDecoder('utf-8').decode(view);
}

/* create a string from the utf-8 encoded data at ptr in the main application */
_state.load_main_string = function (ptr, size) {
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

	/* setup the glue imports */
	let imports = {
		host: {}
	};
	imports.host.host_load_core = function (id, ptr, size) {
		let buffer = _state.make_buffer(ptr, size);
		_state.load_core(id, buffer)
	};
	imports.host.host_get_main_export = function (ptr, size) {
		let name = _state.load_glue_string(ptr, size);
		return _state.main.exports[name];
	};
	imports.host.host_get_core_export = function (instance, ptr, size) {
		let name = _state.load_glue_string(ptr, size);
		return instance.exports[name];
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
			_state.load_main();
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
		throw new Error(`Unexpected termination: [${code}]`);
	};
	imports.env.host_print_u8 = function (ptr, size) {
		console.log(_state.load_main_string(ptr, size));
	}
	imports.env.host_fail_u8 = function (ptr, size) {
		let msg = _state.load_main_string(ptr, size);
		console.error(msg);
		throw new Error(`Application failed`);
	}
	imports.env.ctx_create = _state.glue.exports.ctx_create;
	imports.env.ctx_set_core = _state.glue.exports.ctx_set_core;
	imports.env.ctx_destroy = _state.glue.exports.ctx_destroy;
	imports.env.mem_expand_physical = _state.glue.exports.mem_expand_physical;
	imports.env.mem_move_physical = _state.glue.exports.mem_move_physical;
	imports.env.mem_flush_caches = _state.glue.exports.mem_flush_caches;
	imports.env.mem_read_u8_i32 = _state.glue.exports.mem_read_u8_i32;
	imports.env.mem_read_i8_i32 = _state.glue.exports.mem_read_i8_i32;
	imports.env.mem_read_u16_i32 = _state.glue.exports.mem_read_u16_i32;
	imports.env.mem_read_i16_i32 = _state.glue.exports.mem_read_i16_i32;
	imports.env.mem_read_i32 = _state.glue.exports.mem_read_i32;
	imports.env.mem_read_i64 = _state.glue.exports.mem_read_i64;
	imports.env.mem_read_f32 = _state.glue.exports.mem_read_f32;
	imports.env.mem_read_f64 = _state.glue.exports.mem_read_f64;
	imports.env.mem_write_u8_i32 = _state.glue.exports.mem_write_u8_i32;
	imports.env.mem_write_i8_i32 = _state.glue.exports.mem_write_i8_i32;
	imports.env.mem_write_u16_i32 = _state.glue.exports.mem_write_u16_i32;
	imports.env.mem_write_i16_i32 = _state.glue.exports.mem_write_i16_i32;
	imports.env.mem_write_i32 = _state.glue.exports.mem_write_i32;
	imports.env.mem_write_i64 = _state.glue.exports.mem_write_i64;
	imports.env.mem_write_f32 = _state.glue.exports.mem_write_f32;
	imports.env.mem_write_f64 = _state.glue.exports.mem_write_f64;
	imports.env.mem_execute_u8_i32 = _state.glue.exports.mem_execute_u8_i32;
	imports.env.mem_execute_i8_i32 = _state.glue.exports.mem_execute_i8_i32;
	imports.env.mem_execute_u16_i32 = _state.glue.exports.mem_execute_u16_i32;
	imports.env.mem_execute_i16_i32 = _state.glue.exports.mem_execute_i16_i32;
	imports.env.mem_execute_i32 = _state.glue.exports.mem_execute_i32;
	imports.env.mem_execute_i64 = _state.glue.exports.mem_execute_i64;
	imports.env.mem_execute_f32 = _state.glue.exports.mem_execute_f32;
	imports.env.mem_execute_f64 = _state.glue.exports.mem_execute_f64;

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

			/* notify the glue module about the loaded main application, which will also startup the wasmlator itself (will also call the
			*	required _initialize function; do this in a separate call to prevent exceptions from propagating into the catch-handler) */
			setTimeout(() => {
				console.log(`WasmLator.js: Starting up main module...`);
				_state.glue.exports.host_main_loaded(1);
			});
		})
		.catch((err) => {
			console.error(`WasmLator.js: Failed to load main module: ${err}`);
			_state.glue.exports.host_main_loaded(0);
		});
}

/* load a core module */
_state.load_core = function (id, buffer) {
	console.log(`WasmLator.js: Instantiating core for [${id}]...`);

	/* setup the core imports */
	let imports = {
		memory: {}
	};
	imports.memory.mem_perform_lookup = _state.main.exports.mem_perform_lookup;
	imports.memory.mem_result_physical = _state.main.exports.mem_result_physical;
	imports.memory.mem_result_size = _state.main.exports.mem_result_size;
	imports.memory.mem_mmap = _state.main.exports.mem_mmap;
	imports.memory.mem_munmap = _state.main.exports.mem_munmap;
	imports.memory.mem_mprotect = _state.main.exports.mem_mprotect;

	/* try to instantiate the core module */
	WebAssembly.instantiate(buffer, imports)
		.then((instance) => {
			/* notify the glue module about the loaded core (do this in a separate
			*	call to prevent exceptions from propagating into the catch-handler) */
			setTimeout(() => {
				console.log(`WasmLator.js: Core for [${id}] loaded`);
				_state.glue.exports.host_core_loaded(id, instance.instance);
			});
		})
		.catch((err) => {
			console.error(`WasmLator.js: Failed to load core for [${id}]: ${err}`);
			_state.glue.exports.host_core_loaded(id, null);
		});
}

_state.load_glue();
