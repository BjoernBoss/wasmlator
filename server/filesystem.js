class OpenFile {
	constructor(path) {
		this.path = path;
		this.dirty = false;
		this.exclusive = false;
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

	/* cb(stats): stats either array-buffer encoding stats-object or null, if not found */
	getStats(path, cb) {
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
				return resp.arrayBuffer();
			})
			.then((buf) => {
				this._log(`Stats for [${path}] received`);
				this._cache[path] = buf;
				cb(buf);
			})
			.catch((err) => {
				this._err(`Failed to fetch stats for [${path}]: ${err}`);
				cb(null);
			});
	}

	/* cb() */
	openFile(path, create, truncate, cb) {

	}

	closeAll() {

	}
}
