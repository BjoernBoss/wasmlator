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
		exports: {}
	}
};

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
	console.log(`WasmLator: Loading glue module...`);

	/* setup the glue imports */
	let imports = {
		host: {}
	};
	imports.host.host_load_core = function (id, ptr, size) {
		let buffer = _state.make_buffer(ptr, size);

		console.log(`WasmLator: Instantiating core for [${id}]...`);
		WebAssembly.instantiate(buffer, _state.imports.main)
			.then((instance) => {
				console.log(`WasmLator: Core for [${id}] created successfully`);
				_state.glue.exports.host_core_callback(id, instance);
			})
			.catch((err) => {
				console.error(`WasmLator: Failed to create core for [${id}]: ${err}`);
				_state.glue.exports.host_core_callback(id, null);
			});
	};
	imports.host.host_get_function = function (object, ptr, size) {
		let name = _state.load_string(ptr, size);
		return object[name];
	};

	/* fetch the initial glue module and try to instantiate it */
	fetch(_state.glue.path, { credentials: 'same-origin' })
		.then((resp) => {
			if (!resp.ok)
				throw new Error(`Failed to load [${_state.glue.path}]`);
			return WebAssembly.instantiateStreaming(resp, imports);
		})
		.then((instance) => {
			console.log(`WasmLator: Glue module loaded`);
			_state.glue.wasm = instance.instance;
			_state.glue.exports = _state.glue.wasm.exports;

			/* load the main module, which will then startup the wasmlator */
			_state.load_main();
		})
		.catch((err) => {
			console.error(`Failed to load and instantiate glue module: ${err}`);
		});
}

/* load the actual primary application once the glue module has been loaded and compiled */
_state.load_main = function () {
	console.log(`WasmLator: Loading main application...`);

	/* setup the main imports (env.emscripten_notify_memory_growth and wasi_snapshot_preview1.proc_exit required by wasm-standalone module) */
	let imports = {
		env: {},
		wasi_snapshot_preview1: {}
	};
	imports.env.emscripten_notify_memory_growth = function () { };
	imports.wasi_snapshot_preview1.proc_exit = function (code) {
		console.error(`WasmLator: Main application unexpectedly terminated itself with [${code}]`);
		throw new Error(`Unexpected termination with [${code}]`);
	};
	imports.env.host_log_u8 = function (ptr, size) {
		console.log(`Log: ${_state.load_string(ptr, size)}`);
	}
	imports.env.host_debug_u8 = function (ptr, size) {
		console.log(`Debug: ${_state.load_string(ptr, size)}`);
	}
	imports.env.host_fail_u8 = function (ptr, size) {
		let msg = _state.load_string(ptr, size);
		console.error(`WasmLator: Failure in main application: ${msg}`);
		throw new Error(`Failure with ${msg}`);
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
			console.log(`WasmLator: Main module loaded`);
			_state.main.wasm = instance.instance;
			_state.main.exports = _state.main.wasm.exports;
			_state.main.memory = _state.main.exports.memory;

			/* execute the '_initialize' function, required for standalone wasm-modules without entry-point */
			console.log(`WasmLator: Running main module _initialize()...`);
			_state.main.exports._initialize();
			console.log(`WasmLator: Main module initialization completed`);

			/* start the actual wasmaltor application */
			_state.startup_wasmlastor();
		})
		.catch((err) => {
			console.error(`WasmLator: Failed to load and instantiate main module: ${err}`);
		});
}

/* startup the actual main application */
_state.startup_wasmlastor = function () {
	console.log(`WasmLator: Starting up wasmlator...`);

	/* */
}

_state.load_glue();
