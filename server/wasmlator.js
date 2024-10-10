let _state = {
	main: {
		path: './main.js',
		wasm: null,
		imports: {},
		exports: {}
	},
	glue: {
		path: './.generated/glue-module.wasm',
		wasm: null,
		imports: {
			host: {},
		},
		exports: {}
	}
};

/* create a string from the utf-8 encoded data at ptr in the main application */
_state.load_string = function (ptr, size) {
	let view = new DataView(_state.main.wasm.memory.buffer, ptr, size);
	return new TextDecoder('utf-8').decode(view);
}

/* create a buffer (newly allocated) from the data at ptr in the main application */
_state.make_buffer = function (ptr, size) {
	return _state.main.wasm.memory.buffer.slice(ptr, ptr + size);
}

_state.glue.imports.host.host_load_core = function (id, ptr, size) {
	let buffer = _state.make_buffer(ptr, size);

	console.log(`Instantiating core for [${id}]...`);
	WebAssembly.instantiate(buffer, _state.imports.main)
		.then((instance) => {
			console.log(`Core for [${id}] created successfully`);
			_state.glue.exports.host_core_callback(id, instance);
		})
		.catch((err) => {
			console.error(`Failed to create core for [${id}]: ${err}`);
			_state.glue.exports.host_core_callback(id, null);
		});
}

_state.glue.imports.host.host_get_function = function (object, ptr, size) {
	let name = _state.load_string(ptr, size);
	return object[name];
}

/* load the actual primary application once the glue module has been loaded and compiled */
_state.load_application = function () {
	console.log(`Loading main application...`);

	/* setup the main application imports */
	implement me!

	/* fetch the main application javascript-wrapper */
	fetch(_state.main.path, { credentials: 'same-origin' })
		.then((resp) => {
			if (!resp.ok)
				throw `Failed to load [${_state.main.path}]`;
			return resp.arrayBuffer();
		})
		.then((buffer) => {
			let code = `(function() { ${new TextDecoder('utf-8').decode(buffer)}; return Module; })()`;
			console.log(eval(code));
			console.log(`Main application loaded`);
		})
		.catch((err) => {
			console.error(`Failed to load main application: ${err}`);
		});
}

/* load the initial glue module and afterwards the primary application */
_state.startup = function () {
	console.log(`Loading glue module...`);

	/* fetch the initial glue module and try to instantiate it */
	fetch(_state.glue.path, { credentials: 'same-origin' })
		.then((resp) => {
			if (!resp.ok)
				throw `Failed to load [${_state.glue.path}]`;
			return WebAssembly.instantiateStreaming(resp, _state.glue.imports);
		})
		.then((instance) => {
			console.log(`Glue module loaded`);
			_state.glue.wasm = instance.instance;
			_state.glue.exports = _state.glue.wasm.exports;
			_state.load_application();
		})
		.catch((err) => {
			console.error(`Failed to load and instantiate glue module: ${err}`);
		});
}

_state.startup();
