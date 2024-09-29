
function jsContextCreate() {
	_env.core.push(null);
	return _env.core.length - 1;
}
function jsContextSetCore(id, ptr, size) {
	/* fetch the wasm-code */
	let data = _env.self.asm.memory.buffer.slice(ptr, ptr + size);

	/* construct the imports */
	let imports = {};
	memoryImports(imports);

	/* compile and load the wasm-module */
	_env.core[id] = WebAssembly.instantiate(data, imports);
}
function jsContextDestroy(id) {
	_env.core[id] = null;
}

mergeInto(LibraryManager.library, {
	jsContextCreate,
	jsContextSetCore,
	jsContextDestroy
});
