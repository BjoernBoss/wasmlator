let _state = {
	main: {
		path: './wasm/main.wasm',
		memory: null,
		exports: {}
	},
	glue: {
		path: './wasm/glue-module.wasm',
		memory: null,
		exports: {}
	},
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

/* create a string from the utf-8 encoded data at ptr in the main application or the glue-module */
_state.load_string = function (ptr, size, main_memory) {
	let view = new DataView((main_memory ? _state.main.memory.buffer : _state.glue.memory.buffer), ptr, size);
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
	imports.host.host_get_export = function (instance, ptr, size) {
		let obj = instance.exports[_state.load_string(ptr, size, true)];
		if (obj === undefined)
			return null;
		return obj;
	};
	imports.host.host_get_function = function (instance, ptr, size, main_memory) {
		let obj = instance.exports[_state.load_string(ptr, size, main_memory)];
		if (obj === undefined)
			return null;
		return obj;
	};
	imports.host.host_make_object = function () {
		return {};
	}
	imports.host.host_set_member = function(obj, name, size, value) {
		obj[_state.load_string(name, size, true)] = value;
	}

	/* fetch the initial glue module and try to instantiate it */
	fetch(_state.glue.path, { credentials: 'same-origin' })
		.then((resp) => {
			if (!resp.ok)
				throw new Error(`Failed to load [${_state.glue.path}]`);
			return WebAssembly.instantiateStreaming(resp, imports);
		})
		.then((instance) => {
			console.log(`WasmLator.js: Glue module loaded`);
			_state.glue.exports = instance.instance.exports;
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
	console.log(`WasmLator.js: Loading main module...`);
	let imports = {
		env: {},
		wasi_snapshot_preview1: {}
	};

	/* copy all glue exports as imports of main (only the relevant will be bound) */
	imports.env = { ..._state.glue.exports };

	/* setup the main imports (env.emscripten_notify_memory_growth and wasi_snapshot_preview1.proc_exit required by wasm-standalone module) */
	imports.env.emscripten_notify_memory_growth = function () { };
	imports.wasi_snapshot_preview1.proc_exit = function (code) {
		console.error(`WasmLator.js: Main module unexpectedly terminated itself with [${code}]`);
		_state.abortControlled();
	};

	/* setup the remaining host-imports */
	imports.env.host_print_u8 = function (ptr, size, err) {
		(err ? console.error : console.log)(_state.load_string(ptr, size, true));
	};
	imports.env.host_abort = function () {
		_state.abortControlled();
	};
	imports.env.host_load_core = function (ptr, size, process) {
		return _state.load_core(_state.make_buffer(ptr, size), process);
	};
	imports.env.host_load_block = function (ptr, size, process) {
		return _state.load_block(_state.make_buffer(ptr, size), process);
	};

	/* fetch the main application javascript-wrapper */
	fetch(_state.main.path, { credentials: 'same-origin' })
		.then((resp) => {
			if (!resp.ok)
				throw new Error(`Failed to load [${_state.main.path}]`);
			return WebAssembly.instantiateStreaming(resp, imports);
		})
		.then((instance) => {
			console.log(`WasmLator.js: Main module loaded`);
			_state.main.exports = instance.instance.exports;
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
_state.load_core = function (buffer, process) {
	/* copy all glue/main exports as imports of the core (only the relevant will be bound) */
	let imports = {};
	imports.glue = { ..._state.glue.exports };
	imports.main = { ..._state.main.exports };

	/* try to instantiate the core module */
	WebAssembly.instantiate(buffer, imports)
		.then((instance) => {
			/* notify the main about the loaded core (do this in a separate
			*	call to prevent exceptions from propagating into the catch-handler) */
			_state.controlled(() => {
				/* set the last-instance, invoke the handler, and then reset the last-instance
				*	again, in order to ensure unused instances can be garbage-collected */
				_state.glue.exports.set_last_instance(instance.instance);
				_state.main.exports.main_core_loaded(process, 1);
				_state.glue.exports.set_last_instance(null);
			});
		})
		.catch((err) => {
			_state.controlled(() => {
				console.error(`WasmLator.js: Failed to load core: ${err}`);
				_state.main.exports.main_core_loaded(process, 0);
			});
		});
	return 1;
}

/* load a block module */
_state.load_block = function (buffer, process) {
	/* fetch the imports-object */
	let imports = _state.glue.exports.get_imports();

	/* try to instantiate the block module */
	WebAssembly.instantiate(buffer, imports)
		.then((instance) => {
			/* notify the main about the loaded block (do this in a separate
			*	call to prevent exceptions from propagating into the catch-handler) */
			_state.controlled(() => {
				/* set the last-instance, invoke the handler, and then reset the last-instance
				*	again, in order to ensure unused instances can be garbage-collected */
				_state.glue.exports.set_last_instance(instance.instance);
				_state.main.exports.main_block_loaded(process, 1);
				_state.glue.exports.set_last_instance(null);
			});
		})
		.catch((err) => {
			_state.controlled(() => {
				console.error(`WasmLator.js: Failed to load block: ${err}`);
				_state.main.exports.main_block_loaded(process, 0);
			});
		});
	return 1;
}

_state.load_glue();
