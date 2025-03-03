/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
import { realpathSync, promises as fs } from 'fs';
import * as filePath from 'path';

export class NodeHost {
	constructor(reader, root, wasm, impFileStats, impLogType) {
		this._reader = reader;
		this._root = this._makeRealPath(root);
		this._wasm = this._makeRealPath(wasm);
		this._lastOpenLine = false;
		this._impFileStats = impFileStats;
		this._impLogType = impLogType;
	}

	_makeRealPath(path) {
		path = realpathSync(path);
		if (process.platform == 'win32') {
			if (path.startsWith('\\\\?\\'))
				path = path.substring(4);
			return path.toLowerCase();
		}
		return path;
	}
	async _validatePath(path) {
		/* check if its the root path */
		if (path == '/')
			return [this._root, []];

		/* validate the path itself to be absolute and canonicalized */
		if (!path.startsWith('/') || path.indexOf('\\') != -1)
			return [null, []];
		if (path.endsWith('/'))
			path = path.substring(0, path.length - 1);
		let parts = path.substring(1).split('/');
		if (('' in parts) || ('.' in parts) || ('..' in parts))
			return [null, []];

		/* iterate over the path components and ensure that each is a valid directory (i.e. not a symlink) */
		let actual = this._root;
		for (let i = 0; i < parts.length - 1; ++i) {
			actual = filePath.join(actual, parts[i]);
			try {
				const stats = await fs.lstat(actual);
				if (stats.isSymbolicLink() || !stats.isDirectory())
					return [null, []];
			}
			catch (err) {
				return [null, []];
			}
		}
		return [filePath.join(actual, parts[parts.length - 1]), parts];
	}
	_patchLink(parts, link) {
		/* check if the link is absolute and matches the base-path of the file-system */
		if (filePath.isAbsolute(link)) {
			try {
				link = this._makeRealPath(link);
				if (!link.startsWith(this._root) || !('/\\').includes(link[this._root.length]))
					return null;
				return '/' + filePath.relative(this._root, link).replaceAll('\\', '/');
			}
			catch (_) {
				return null;
			}
		}

		/* check if the path is relative but does not leave the root directory */
		link = link.replaceAll('\\', '/');
		if (link.length == 0 || link.startsWith('/'))
			return null;
		if (link.endsWith('/'))
			link = link.substring(0, link.length - 1);
		let lparts = link.split('/');

		/* apply the relative changes to the current parts into the current directory */
		for (const part of lparts) {
			if (part == '')
				return null;
			if (part == '.')
				continue
			if (part != '..')
				parts.push(part);
			else if (parts.length == 0)
				return null;
			else
				parts.pop();
		}
		return link;
	}

	log(type, msg) {
		/* check if its a normal output-log, which can simply be written to the stdout */
		if (type == this._impLogType.output) {
			this._lastOpenLine = true;
			process.stdout.write(msg);
		}

		/* ensure a clean line-break and write the error to the stderr */
		else if (type == this._impLogType.fatal || type == this._impLogType.errInternal) {
			msg = msg.substring(0, msg.length - 1);
			if (this._lastOpenLine)
				process.stdout.write('\n');
			this._lastOpenLine = false;
			console.error(msg);
		}
	}
	async loadGlue(imports) {
		let data = await fs.readFile(`${this._wasm}/glue.wasm`);
		let instantiated = await WebAssembly.instantiate(data, imports);
		return instantiated.instance;
	}
	async loadMain(imports) {
		let data = await fs.readFile(`${this._wasm}/main.wasm`);
		let instantiated = await WebAssembly.instantiate(data, imports);
		return instantiated.instance;
	}
	async loadModule(imports, buffer) {
		let module = await WebAssembly.compile(buffer);
		let instance = await WebAssembly.instantiate(module, imports);
		return instance;
	}
	async readInput() {
		let _that = this;
		return new Promise(function (resolve) {
			_that._reader.question('', function (m) {
				resolve(m + '\n');
			});
		});
	}
	async fsLoadStats(path) {
		/* validate the filepath */
		var [actual, parts] = await this._validatePath(path);
		if (actual == null)
			return null;

		/* read the stats of the path */
		let stats = null;
		try {
			stats = await fs.lstat(actual);
		} catch (_) {
			return null;
		}

		/* parse the stats into the output structure */
		let out = new this._impFileStats('');
		out.atime_us = stats.atime.getTime() * 1000;
		out.mtime_us = stats.mtime.getTime() * 1000;

		/* check if the object is a symbolic link */
		if (stats.isSymbolicLink()) {
			out.type = 'link';

			/* validate the link */
			let link = null;
			try {
				link = this._patchLink(parts, await fs.readlink(actual));
			}
			catch (err) { }
			if (link == null)
				return null;
			out.link = link;
			out.size = new TextEncoder().encode(out.link).length;
		}

		/* check if the object is a regular file */
		else if (stats.isFile()) {
			out.type = 'file';
			out.size = stats.size;
		}

		/* check if the object is a directory */
		else if (stats.isDirectory())
			out.type = 'dir';

		/* otherwise unknown type */
		else
			return null;
		return out;
	}
	async fsLoadData(path) {
		/* validate the filepath */
		var [actual, _] = await this._validatePath(path);
		if (actual == null)
			throw new Error(`Invalid path [${path}] encountered`);
		let stats = await fs.lstat(actual);
		if (stats.isSymbolicLink() || !stats.isFile())
			throw new Error(`Invalid file [${path}] encountered`);

		/* read the content of the file */
		return new Uint8Array(await fs.readFile(actual));
	}
	async fsLoadChildren(path) {
		/* validate the filepath */
		var [actual, _] = await this._validatePath(path);
		if (actual == null)
			throw new Error(`Invalid path [${path}] encountered`);
		let stats = await fs.lstat(actual);
		if (stats.isSymbolicLink() || !stats.isDirectory())
			throw new Error(`Invalid directory [${path}] encountered`);

		/* read the content of the file and sanitize it */
		let list = await fs.readdir(actual);
		return list.filter((v) => !['', '.', '..'].includes(v));
	}
}
