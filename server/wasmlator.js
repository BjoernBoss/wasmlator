class BusyError extends Error { constructor(m) { super(m); this.name = ''; } };

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
		completed: null,
		fs: null
	};

	/* interact with the busy-areas (to prevent nested execution of commands) */
	_state.start = function (cb) {
		if (_state.loadState != 'none' && _state.loadState != 'loaded')
			throw new BusyError('Wasmlator.js: Failed to be loaded properly and is in an inoperable state');
		if (_state.busy > 0)
			throw new BusyError('Wasmlator.js: Currently busy with another operation');

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
		if (_state.loadState == 'loaded')
			_state.selfLog('Loaded successfully and ready...');
		else
			_state.selfFail('Failed to load properly...');
		if (cb != null)
			cb();
	});
	_state.loadState = 'loading';

	/* execute the callback in a catch-handler to log any potential unhandled exceptions being thrown */
	class PrintStack extends Error { constructor(m) { super(m); this.name = ''; } };
	_state.controlled = function (fn) {
		try {
			fn();
		} catch (e) {
			_state.selfFail(`Unhandled exception occurred: ${e.stack}`);
		}
	}
	_state.selfLog = function (msg) {
		console.log(`WasmLator.js: ${msg}`);
	};
	_state.selfFail = function (msg) {
		logPrint(`F:WasmLator.js: ${msg}`);
	};

	/* setup the new filesystem host */
	_state.fs = new FSHost((msg) => _state.selfLog(msg), (msg) => _state.selfFail(msg));

	/* create a string from the utf-8 encoded data at ptr in the main application or the glue-module */
	_state.load_string = function (ptr, size, main_memory) {
		let view = new DataView((main_memory ? _state.main.memory : _state.glue.memory), ptr, size);
		return new TextDecoder('utf-8').decode(view);
	};

	/* create a buffer (newly allocated) from the data at ptr in the main application */
	_state.make_buffer = function (ptr, size) {
		return _state.main.memory.slice(ptr, ptr + size);
	};

	/* helper to write the result of a task to the application */
	_state.task_completed = function (process, payload) {
		let addr = 0, size = 0;

		/* write the result to the main application */
		if (payload != null) {
			let buffer = new TextEncoder('utf-8').encode(JSON.stringify(payload));
			addr = _state.main.exports.main_allocate(buffer.byteLength);
			size = buffer.byteLength;
			new Uint8Array(_state.main.memory, addr, size).set(new Uint8Array(buffer));
		}

		/* invoke the callback and mark the critical section as completed */
		_state.controlled(() => _state.main.exports.main_task_completed(process, addr, size));
		_state.leave();
	};

	/* task dispatcher for the primary application */
	_state.handle_task = function (task, process) {
		/* enter the callback and leave it once the command has been completed */
		_state.enter();

		/* extract the next command */
		let cmd = task, i = 0, payload = '';
		if ((i = task.indexOf(':')) >= 0) {
			cmd = task.substring(0, i);
			payload = task.substring(i + 1);
		}

		/* handle the core and block creation handling */
		if (cmd == 'core') {
			/* close all files, as there might have been some previously opened files remaining from the last core */
			_state.fs.closeAll();
			let args = payload.split(':');
			_state.load_core(_state.make_buffer(parseInt(args[0]), parseInt(args[1])), process);
		}
		else if (cmd == 'block') {
			let args = payload.split(':');
			_state.load_block(_state.make_buffer(parseInt(args[0]), parseInt(args[1])), process);

		}
		/* handle the file-system commands */
		else if (cmd == 'stats')
			_state.fs.getStats(payload, (s) => _state.task_completed(process, s));
		else if (cmd == 'opexisting')
			_state.fs.openFile(payload, false, true, false, (id, s) => _state.task_completed(process, { id: id, stats: s }));

		/* default catch-handler for unknown commands */
		else
			_state.selfFail(new PrintStack(`Received unknown task [${cmd}]`).stack);
	};

	/* load the initial glue module and afterwards the main application */
	_state.load_glue = function () {
		_state.selfLog(`Loading glue module...`);

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
				_state.selfLog(`Glue module loaded`);
				_state.glue.exports = instance.instance.exports;
				_state.glue.memory = _state.glue.exports.memory.buffer;

				/* load the main module, which will then startup the wasmlator */
				_state.controlled(() => _state.load_main());
			})
			.catch((err) => _state.selfFail(`Failed to load glue module: ${err}`))

			/* leave the busy-area (may execute the completed callback - no matter if the controlled execution succeeded or failed) */
			.finally(() => _state.leave());
	};

	/* load the actual primary application once the glue module has been loaded and compiled */
	_state.load_main = function () {
		_state.selfLog(`Loading main module...`);

		/* enter the busy-area (will be left once the glue module has been loaded successfully or failed) */
		_state.enter();
		let imports = {
			env: {},
			wasi_snapshot_preview1: {}
		};

		/* copy all glue exports as imports of main (only the relevant will be bound) */
		imports.env = { ..._state.glue.exports };

		/* setup the main imports (env.emscripten_notify_memory_growth and wasi_snapshot_preview1.proc_exit required by wasm-standalone module) */
		imports.env.emscripten_notify_memory_growth = function () {
			_state.main.memory = _state.main.exports.memory.buffer;
		};
		imports.wasi_snapshot_preview1.proc_exit = function (code) {
			_state.selfFail(new PrintStack(`Main module terminated itself with exit-code [${code}] - (Unhandled exception?)`).stack);
		};

		/* setup the remaining host-imports */
		imports.env.host_task = function (ptr, size, process) {
			_state.handle_task(_state.load_string(ptr, size, true), process);
		};
		imports.env.host_message = function (ptr, size) {
			logPrint(_state.load_string(ptr, size, true));
		};
		imports.env.host_failure = function (ptr, size) {
			logPrint(new PrintStack(_state.load_string(ptr, size, true)).stack);
		};
		imports.env.host_random = function () {
			return Math.floor(Math.random() * 0x1_0000_0000);
		};

		/* fetch the main application javascript-wrapper */
		fetch(_state.main.path, { credentials: 'same-origin' })
			.then((resp) => {
				if (!resp.ok)
					throw new Error(`Failed to load [${_state.main.path}]`);
				return WebAssembly.instantiateStreaming(resp, imports);
			})
			.then((instance) => {
				_state.selfLog(`Main module loaded`);
				_state.main.exports = instance.instance.exports;
				_state.main.memory = _state.main.exports.memory.buffer;

				/* startup the main application, which requires the internal _initialize to be invoked */
				_state.controlled(() => {
					_state.selfLog(`Starting up main module...`);
					_state.main.exports._initialize();
					_state.loadState = 'loaded';
					_state.selfLog(`Main module initialized`);
				});
			})
			.catch((err) => _state.selfFail(`Failed to load main module: ${err}`))

			/* leave the busy-area (may execute the completed callback - no matter if the controlled execution succeeded or failed) */
			.finally(() => _state.leave());
	};

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
				_state.task_completed(process);
				_state.glue.exports.set_last_instance(null);
			}))
			.catch((err) => _state.selfFail(`Failed to load core: ${err}`))

			/* leave the busy-area (may execute the completed callback - no matter if the controlled execution succeeded or failed) */
			.finally(() => _state.leave());
	};

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
				_state.task_completed(process);
				_state.glue.exports.set_last_instance(null);
			}))
			.catch((err) => _state.selfFail(`Failed to load block: ${err}`))

			/* leave the busy-area (may execute the completed callback - no matter if the controlled execution succeeded or failed) */
			.finally(() => _state.leave());
	};

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
			let ptr = _state.main.exports.main_allocate(buf.length);
			new Uint8Array(_state.main.memory, ptr, buf.length).set(buf);

			/* perform the actual execution of the command (will ensure to free it) */
			_state.main.exports.main_user_command(ptr, buf.length);
		});

		/* leave the busy-section (may execute the callback) */
		_state.leave();
	};
}
