class FileNode {
	constructor(stats) {
		this.dataDirty = false;
		this.metaDirty = false;
		this.stats = stats;
		this.users = 0;
		this.data = null;
	}

	addUser() {
		if (this.users == 0)
			this.data = new ArrayBuffer(0, { maxByteLength: 100_000_000 });
		this.users += 1;
	}
	read() {
		this.metaDirty = true;
		this.stats.atime_us = Date.now() * 1000;
	}
	written() {
		this.dataDirty = true;
		this.metaDirty = true;
		this.stats.mtime_us = Date.now() * 1000;
	}
}

class FSHost {
	constructor(log, err) {
		this._log = log;
		this._err = err;

		/* set of all open or queried files */
		this._files = {};

		/* map of ids to file nodes */
		this._open = [];
	}

	_getStats(path, cb) {
		/* check if the path exists in the cache */
		if (path in this._files) {
			cb(this._files[path].stats);
			return;
		}
		this._log(`Fetching stats for [${path}]...`);

		/* request the stats */
		let actual = `/stat${path}`;
		fetch(actual, { credentials: 'same-origin' })
			.then((resp) => {
				if (!resp.ok)
					throw new Error(`Failed to load [${path}]`);
				return resp.json();
			})
			.then((json) => {
				this._log(`Stats for [${path}] received`);
				this._files[path] = new FileNode(json);
				cb(json);
			})
			.catch((err) => {
				this._err(`Failed to fetch stats for [${path}]: ${err}`);
				cb(null);
			});
	}
	_openFile(stats, path, create, open, truncate, cb) {
		/* validate already existing objects */
		if (stats != null) {
			/* check if the type is valid */
			if (stats.type != 'file') {
				cb(null, stats);
				return;
			}

			/* check if an existing file can be opened */
			if (!open) {
				cb(null, stats);
				return;
			}
		}

		/* check if the file can be created */
		else if (!create) {
			cb(null, stats);
			return;
		}

		/* lookup the corresponding file-node */
		let node = null;
		if (path in this._files)
			node = this._files[path];

		/* check if the file should be truncated or a new file created */
		if (truncate || stats == null) {
			/* check if the file-node needs to be created */
			if (stats == null) {
				stats = {};
				stats.name = path.substr(path.lastIndexOf('/') + 1);
				stats.link = '';
				stats.size = 0;
				stats.atime_us = Date.now() * 1000;
				stats.mtime_us = stats.atime_us;
				stats.type = 'file';
				node = (this._files[path] = new FileNode(stats));
				node.addUser();
			}

			/* truncate the existing file */
			else {
				if (node == null)
					node = (this._files[path] = new FileNode(stats));
				node.addUser();

				if (node.data.byteLength > 0 || node.stats.size > 0) {
					node.stats.size = 0;
					node.data.resize(0);
					node.written();
				}
			}

			/* allocate the index in the open-map */
			this._open.push(node);

			/* notify the callback about the opened file */
			cb(this._open.length - 1, node.stats);
			return;
		}

		/* check if the file does not need to be read again */
		if (node.data != null) {
			node.addUser();
			this._open.push(node);
			cb(this._open.length - 1, node.stats);
			return;
		}

		/* fetch the existing file data */
		this._log(`Reading file [${path}]...`);

		/* request the stats */
		let actual = `/data${path}`;
		fetch(actual, { credentials: 'same-origin' })
			.then((resp) => {
				if (!resp.ok)
					throw new Error(`Failed to load [${path}]`);
				return resp.arrayBuffer();
			})
			.then((buf) => {
				this._log(`Data of [${path}] received`);

				/* setup the new file-node */
				if (node == null)
					node = (this._files[path] = new FileNode(stats));
				node.addUser();
				node.stats.size = buf.byteLength;
				this._open.push(node);

				/* write the data to the node */
				node.data.resize(buf.byteLength);
				new Uint8Array(node.data).set(new Uint8Array(buf));
				cb(this._open.length - 1, node.stats);
			})
			.catch((err) => {
				this._err(`Failed to fetch stats for [${path}]: ${err}`);
				cb(null, stats);
			});
	}

	/* cb(stats): stats either stats-object or null, if not found */
	getStats(path, cb) {
		this._getStats(path, cb);
	}

	/* cb(id, stats): id if succedded, else null, stats if exists, else null */
	openFile(path, create, open, truncate, cb) {
		/* fetch the stats of the file */
		this._getStats(path, (s) => {
			/* check if the stats were found */
			if (s != null) {
				this._openFile(s, path, create, open, truncate, cb);
				return;
			}

			/* check if the parent directory exists */
			let i = path.lastIndexOf('/');
			if (i <= 0) {
				cb(null, null);
				return;
			}
			this._getStats(path.substr(0, i), (s) => {
				/* check if the directory exists, and otherwise fail the call */
				if (s == null) {
					cb(null, null);
					return;
				}

				/* create the new file */
				this._openFile(null, path, create, open, truncate, cb);
			});
		});
	}

	/* cb(read): returns number of bytes read or null on failure */
	readFile(id, buffer, offset, cb) {
		/* validate the id */
		if (id >= this._open.length || this._open[id] == null) {
			cb(0);
			return;
		}

		/* compute the number of bytes to read */
		let count = Math.min(buffer.byteLength, this._open[id].data.byteLength - offset);
		if (count <= 0) {
			cb(0);
			return;
		}

		/* read the data to the buffer */
		buffer.set(new Uint8Array(this._open[id].data, offset, count));

		/* update the meta-data and notify the caller */
		this._open[id].read();
		cb(count);
	}

	closeAll() {

	}
}
