import { FileStats, LogType } from '../generated/common.js';
import { readFile, stat } from 'fs';

export class NodeHost {
	constructor(reader) {
		this._reader = reader;
	}

	log(type, msg) {
		msg = msg.substring(0, msg.length - 1);
		if (type == LogType.output)
			console.log(msg);
		else if (type == LogType.fatal || type == LogType.errInternal)
			console.error(msg);
	}
	async loadGlue(imports) {
		return new Promise(function (resolve, reject) {
			readFile('../generated/glue.wasm', async function (err, data) {
				/* check if the file content could be read */
				if (err != null) {
					reject(`failed to read [glue.wasm]: ${err}`);
					return;
				}

				/* instantiate the module */
				let instantiated = await WebAssembly.instantiate(data, imports);
				return resolve(instantiated.instance);
			});
		});
	}
	async loadMain(imports) {
		return new Promise(function (resolve, reject) {
			readFile('../generated/wasmlator.wasm', async function (err, data) {
				/* check if the file content could be read */
				if (err != null) {
					reject(`failed to read [wasmlator.wasm]: ${err}`);
					return;
				}

				/* instantiate the module */
				let instantiated = await WebAssembly.instantiate(data, imports);
				return resolve(instantiated.instance);
			});
		});
	}
	async loadModule(imports, buffer) {
		let instantiated = await WebAssembly.instantiate(buffer, imports);
		return instantiated.instance;
	}
	async readInput() {
		let _that = this;
		return new Promise(function (resolve) {
			_that._reader.question('', (m) => resolve(m));
		});
	}
	async fsLoadStats(path) {
		return new Promise(function (resolve, reject) {
			stat(path, function (err, stats) {

			});
		});

		//let response = await fetch(`/stat${path}`, { credentials: 'same-origin' });
		//let json = await response.json();
		//if (json == null)
		return null;

		///* populate the filestats with the values received from the remote */
		//let stats = new FileStats(json.type as string);
		//stats.link = (json.link as string);
		//stats.size = (json.size as number);
		//stats.atime_us = (json.atime_us as number);
		//stats.mtime_us = (json.mtime_us as number);
		//return stats;
	}
	async fsLoadData(path) {
		return new Promise(function (resolve, reject) {
			readFile(`../fs${path}`, async function (err, data) {
				/* check if the file content could be read */
				if (err != null) {
					reject(`failed to read [${path}]: ${err}`);
					return;
				}
				return resolve(data);
			});
		});
	}
}
