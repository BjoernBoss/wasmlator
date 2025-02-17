import { FileStats, LogType } from '../generated/common.js';
import * as fs from 'fs';
import * as filePath from 'path';

export class NodeHost {
	constructor(reader, root) {
		this._reader = reader;
		this._root = this._makeRealPath(root);
		this._lastOpenLine = false;
	}

	_makeRealPath(path) {
		path = fs.realpathSync(path);
		if (process.platform == 'win32') {
			if (path.startsWith('\\\\?\\'))
				path = path.substring(4);
			return path.toLowerCase();
		}
		return path;
	}
	_validatePath(path) {
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
				const stats = fs.lstatSync(actual);
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
		if (type == LogType.output) {
			this._lastOpenLine = true;
			process.stdout.write(msg);
		}

		/* ensure a clean line-break and write the error to the stderr */
		else if (type == LogType.fatal || type == LogType.errInternal) {
			msg = msg.substring(0, msg.length - 1);
			if (this._lastOpenLine)
				process.stdout.write('\n');
			this._lastOpenLine = false;
			console.error(msg);
		}
	}
	async loadGlue(imports) {
		return new Promise(function (resolve, reject) {
			fs.readFile('generated/glue.wasm', async function (err, data) {
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
			fs.readFile('generated/wasmlator.wasm', async function (err, data) {
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
			_that._reader.question('', function (m) {
				resolve(m + '\n');
			});
		});
	}
	async fsLoadStats(path) {
		let _that = this;
		return new Promise(function (resolve) {
			/* validate the filepath */
			var [actual, parts] = _that._validatePath(path);
			if (actual == null) {
				resolve(null);
				return;
			}

			/* read the stats of the path */
			fs.lstat(actual, function (err, stats) {
				/* check if the stats could be fetched */
				if (err != null) {
					resolve(null);
					return;
				}

				/* parse the stats into the output structure */
				let out = new FileStats('');
				out.atime_us = stats.atime.getTime() * 1000;
				out.mtime_us = stats.mtime.getTime() * 1000;

				/* check if the object is a symbolic link */
				if (stats.isSymbolicLink()) {
					out.type = 'link';

					/* validate the link */
					let link = null;
					try {
						link = _that._patchLink(parts, fs.readlinkSync(actual));
					}
					catch (err) { }
					if (link == null) {
						resolve(null);
						return;
					}
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
				else {
					resolve(null);
					return;
				}
				resolve(out);
			});
		});
	}
	async fsLoadData(path) {
		let _that = this;
		return new Promise(function (resolve, reject) {
			/* validate the filepath */
			var [actual, _] = _that._validatePath(path);
			if (actual == null) {
				reject(`invalid path [${path}] encountered`);
				return;
			}

			/* read the content of the file */
			fs.readFile(actual, async function (err, data) {
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
