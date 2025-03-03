/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
import { FileStats, LogType } from '/com/common.js';

export class WebHost {
	constructor(logger, userInput) {
		this._logger = logger;
		this._userInput = userInput;
	}

	log(type, msg) {
		this._logger(LogType[type], msg);
	}
	async loadGlue(imports) {
		let response = await fetch('/wasm/glue.wasm', { credentials: 'same-origin' });
		let instantiated = await WebAssembly.instantiateStreaming(response, imports);
		return instantiated.instance;
	}

	async loadMain(imports) {
		let response = await fetch('/wasm/main.wasm', { credentials: 'same-origin' });
		let instantiated = await WebAssembly.instantiateStreaming(response, imports);
		return instantiated.instance;
	}
	async loadModule(imports, buffer) {
		let module = await WebAssembly.compile(buffer);
		let instance = await WebAssembly.instantiate(module, imports);
		return instance;
	}
	async readInput() {
		return (await this._userInput()) + '\n';
	}
	async fsLoadStats(path) {
		let response = await fetch(`/stat${path}`, { credentials: 'same-origin' });
		let json = await response.json();
		if (json == null)
			return null;

		/* populate the filestats with the values received from the remote */
		let stats = new FileStats(json.type);
		stats.link = json.link;
		stats.size = json.size;
		stats.atime_us = json.atime_us;
		stats.mtime_us = json.mtime_us;
		return stats;
	}
	async fsLoadData(path) {
		let response = await fetch(`/data${path}`, { credentials: 'same-origin' });
		return new Uint8Array(await response.arrayBuffer());
	}
	async fsLoadChildren(path) {
		let response = await fetch(`/list${path}`, { credentials: 'same-origin' });
		let json = await response.json();

		/* read the content of the file and sanitize it */
		return json.filter((v) => !['', '.', '..'].includes(v));
	}
}
