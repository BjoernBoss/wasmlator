class OpenFile {
	constructor(path) {
		this.path = path;
		this.users = 1;
		this.dirty = false;
		this.data = new ArrayBuffer(0, { maxByteLength: 100_000_000 });
	}
}

class FSHost {
	constructor(log, err) {
		this._log = log;
		this._err = err;

		/* list of all open files */
		this._open = [];

		/* stats-cache (all raw array buffers) */
		this._cache = {};
	}

	_getStats(path, cb) {
		/* check if the path exists in the cache */
		if (path in this._cache) {
			cb(this._cache[path]);
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
				this._cache[path] = json;
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

		/* check if the file is already open */
		let index = null;
		for (let i = 0; i < this._open.length; ++i) {
			if (this._open[i].path != path)
				continue;
			index = i;
			break;
		}

		/* check if the file should be truncated or a new file created */
		if (truncate || stats == null) {
			/* check if the file already exists and reset it */
			if (index != null) {
				if (this._open[index].data.byteLength > 0)
					this._open[index].dirty = true;
				this._open[index].data.resize(0);
				this._open[index].users += 1;
			}

			/* create the completely new file and stats */
			else {
				/* setup the new stats */
				if (stats == null) {
					stats = (this._cache[path] = {});
					stats.name = path.substr(path.lastIndexOf('/') + 1);
					stats.link = '';
					stats.size = 0;
					stats.atime_us = Date.now() * 1000;
					stats.mtime_us = stats.atime;
					stats.type = 'file';
				}

				/* create the new empty entry */
				index = this._open.length;
				this._open.push(new OpenFile(path));
				this._open[index].dirty = (stats.size > 0);
			}

			/* notify the callback about the opened file */
			cb(index, stats);
			return;
		}

		/* check if the file does not need to be read again */
		if (index != null) {
			this._open[index].users += 1;
			cb(index, stats);
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

				/* update the stats and write the data to the file entry */
				stats.size = buf.byteLength;
				index = this._open.length;
				this._open.push(new OpenFile(path));
				this._open[index].data.resize(stats.size);
				new Uint8Array(this._open[index].data).set(new Uint8Array(buf));
				cb(index, stats);
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

	closeAll() {

	}
}
