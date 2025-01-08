class BusyError extends Error { constructor(m) { super(m); this.name = 'BusyError'; } };

setup_wasmlator = function (logPrint, cb) {
	/* local state of all loaded wasmlator-content */
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
		loadState: 'none', // none, loading, loaded
		busy: 0,
		completed: null
	};

	/* interact with the busy-areas (to prevent nested execution of commands) */
	_state.start = function (cb) {
		if (_state.loadState != 'none' && _state.loadState != 'loaded')
			throw new BusyError('Wasmlator.js failed to be loaded properly and is in an inoperable state');
		if (_state.busy > 0)
			throw new BusyError('Wasmlator.js is currently busy with another operation');

		/* setup the busy-state and configure the completed-callback */
		_state.busy = 1;
		_state.completed = cb;
	}
	_state.enter = function () {
		++_state.busy;
	}
	_state.leave = function () {
		if (--_state.busy > 0)
			return;
		if (_state.completed != null)
			_state.completed();
	}

	/* start the official busy-area, as the creation is considered being busy */
	_state.start(() => {
		if (_state.loadState == 'loaded') {
			_state.log('Wasmlator.js: Note: system might not work correctly anymore after a fatal error occurred');
			_state.log('Wasmlator.js: Loaded successfully and ready...');
		}
		else
			_state.failure('Wasmlator.js: Failed to load properly...', true, false);
		if (cb != null)
			cb();
	});
	_state.loadState = 'loading';

	/* execute the callback in a catch-handler to log any potential unhandled exceptions being thrown */
	class PrintError extends Error { constructor(m) { super(m); this.name = ''; } };
	_state.controlled = function (fn) {
		try {
			fn();
		} catch (e) {
			_state.failure(`Wasmlator.js: Unhandled exception occurred: ${e.stack}`, true, true);
		}
	}
	_state.log = function (msg) {
		logPrint(`L:${msg}`);
	};
	_state.failure = function (msg, addHint, withStack) {
		if (withStack)
			logPrint(`${addHint ? 'F:' : ''}${new PrintError(msg).stack}`);
		else
			logPrint(`${addHint ? 'F:' : ''}${msg}`);
	};

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
		_state.log(`WasmLator.js: Loading glue module...`);

		/* enter the busy-area (will be left once the glue module has been loaded successfully or failed) */
		_state.enter();

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
		imports.host.host_set_member = function (obj, name, size, value) {
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
				_state.log(`WasmLator.js: Glue module loaded`);
				_state.glue.exports = instance.instance.exports;
				_state.glue.memory = _state.glue.exports.memory;

				/* load the main module, which will then startup the wasmlator */
				_state.controlled(() => _state.load_main());
			})
			.catch((err) => _state.failure(`Wasmlator.js: Failed to load glue module: ${err}`, true, false))

			/* leave the busy-area (may execute the completed callback - no matter if the controlled execution succeeded or failed) */
			.finally(() => _state.leave());
	}

	/* load the actual primary application once the glue module has been loaded and compiled */
	_state.load_main = function () {
		_state.log(`WasmLator.js: Loading main module...`);

		/* enter the busy-area (will be left once the glue module has been loaded successfully or failed) */
		_state.enter();
		let imports = {
			env: {},
			wasi_snapshot_preview1: {}
		};

		/* copy all glue exports as imports of main (only the relevant will be bound) */
		imports.env = { ..._state.glue.exports };

		/* setup the main imports (env.emscripten_notify_memory_growth and wasi_snapshot_preview1.proc_exit required by wasm-standalone module) */
		imports.env.emscripten_notify_memory_growth = function () { };
		imports.wasi_snapshot_preview1.proc_exit = function (code) {
			_state.failure(`WasmLator.js: Main module terminated itself with exit-code [${code}] - (Unhandled exception?)`, true, true);
		};

		/* setup the remaining host-imports */
		imports.env.host_print_u8 = function (ptr, size, failure) {
			let msg = _state.load_string(ptr, size, true);
			if (failure)
				_state.failure(msg, false, true);
			else
				logPrint(msg);
		};
		imports.env.host_random = function () {
			return Math.floor(Math.random() * 0x1_0000_0000);
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
				_state.log(`WasmLator.js: Main module loaded`);
				_state.main.exports = instance.instance.exports;
				_state.main.memory = _state.main.exports.memory;

				/* startup the main application, which requires the internal _initialize to be invoked */
				_state.controlled(() => {
					_state.log(`WasmLator.js: Starting up main module...`);
					_state.main.exports._initialize();
					_state.loadState = 'loaded';
					_state.log(`WasmLator.js: Main module initialized`);
				});
			})
			.catch((err) => _state.failure(`Wasmlator.js: Failed to load main module: ${err}`, true, false))

			/* leave the busy-area (may execute the completed callback - no matter if the controlled execution succeeded or failed) */
			.finally(() => _state.leave());
	}

	/* load a core module */
	_state.load_core = function (buffer, process) {
		/* enter the busy-area (will be left once the module was loaded successfully; otherwise it will fail anyways) */
		_state.enter();

		/* copy all glue/main exports as imports of the core (only the relevant will be bound) */
		let imports = {};
		imports.glue = { ..._state.glue.exports };
		imports.main = { ..._state.main.exports };

		/* try to instantiate the core module */
		WebAssembly.instantiate(buffer, imports)
			.then((instance) => _state.controlled(() => {
				/* set the last-instance, invoke the handler, and then reset the last-instance
				*	again, in order to ensure unused instances can be garbage-collected */
				_state.glue.exports.set_last_instance(instance.instance);
				_state.main.exports.main_core_loaded(process);
				_state.glue.exports.set_last_instance(null);
			}))
			.catch((err) => _state.failure(`WasmLator.js: Failed to load core: ${err}`, true, false))

			/* leave the busy-area (may execute the completed callback - no matter if the controlled execution succeeded or failed) */
			.finally(() => _state.leave());
		return 1;
	}

	/* load a block module */
	_state.load_block = function (buffer, process) {
		/* enter the busy-area (will be left once the module was loaded successfully or an error occurred) */
		_state.enter();

		/* fetch the imports-object */
		let imports = _state.glue.exports.get_imports();

		/* try to instantiate the block module */
		WebAssembly.instantiate(buffer, imports)
			.then((instance) => _state.controlled(() => {
				/* set the last-instance, invoke the handler, and then reset the last-instance
				*	again, in order to ensure unused instances can be garbage-collected */
				_state.glue.exports.set_last_instance(instance.instance);
				_state.main.exports.main_block_loaded(process);
				_state.glue.exports.set_last_instance(null);
			}))
			.catch((err) => _state.failure(`WasmLator.js: Failed to load block: ${err}`, true, false))

			/* leave the busy-area (may execute the completed callback - no matter if the controlled execution succeeded or failed) */
			.finally(() => _state.leave());
		return 1;
	}

	/* start loading the glue-module (will also load the main-application) */
	_state.load_glue();

	/* mark the system as not busy anymore and return the fuction to pass
	*	string-commands to the main application (may immediately execute the callback) */
	_state.leave();
	return function (command, cb) {
		/* start the next busy-section of the command (any nested deferred calls will increment the busy-counter) */
		_state.start(cb);

		/* pass the actual command to the application */
		_state.controlled(() => {
			/* convert the string to a buffer */
			let buf = new TextEncoder('utf-8').encode(command);

			/* allocate a buffer for the string in the main-application and write the string to it */
			let ptr = _state.main.exports.main_allocate_command(buf.length);
			new Uint8Array(_state.main.memory.buffer, ptr, buf.length).set(buf);

			/* perform the actual execution of the command (will ensure to free it) */
			_state.main.exports.main_command(ptr, buf.length);
		});

		/* leave the busy-section (may execute the callback) */
		_state.leave();
	}
}
