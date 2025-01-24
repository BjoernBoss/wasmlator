class FileNode {
	constructor(ancestor, id, name) {
		this.children = {};
		this.ancestor = ancestor;
		this.name = name;
		this.modified = false;
		this.data = null;
		this.timeout = null;
		this.id = id;
		this.stats = null;
	}
	setupStats(stats, access) {
		this.stats = stats;
		this.stats.id = this.id;
		this.stats.owner = access.owner;
		this.stats.group = access.group;
		this.stats.permissions = access[this.stats.type];
	}
	setupEmptyStats(type, access) {
		this.stats = {};
		this.stats.link = '';
		this.stats.size = 0;
		this.stats.atime_us = Date.now() * 1000;
		this.stats.mtime_us = this.stats.atime_us;
		this.stats.type = type;
		this.stats.id = this.id;
		this.stats.owner = access.owner;
		this.stats.group = access.group;
		this.stats.permissions = access[type];
	}
	read() {
		this.stats.atime_us = Date.now() * 1000;
	}
	written() {
		this.modified = true;
		this.stats.mtime_us = Date.now() * 1000;
	}
}

class MemFileSystem {
	static fsRoot = {
		owner: 0,
		group: 0,
		dir: 0o755
	};
	static fsDefault = {
		owner: 1001,
		group: 1001,
		dir: 0o755,
		file: 0o754,
		link: 0o754
	};

	constructor(log, err) {
		this._log = log;
		this._err = err;

		/* setup the root node */
		this._root = new FileNode(null, 0, '');
		this._root.setupEmptyStats('dir', MemFileSystem.fsRoot);

		/* map of all files */
		this._nodes = [this._root];
	}

	_getNode(node, current, path, cb) {
		/* check if the end has been reached */
		if (path.length == 0 || path == '/')
			return cb(node);

		/* strip the next part of the name and advance the path */
		if (path.startsWith('/'))
			path = path.substr(1);
		let index = path.indexOf('/'), name = '';
		if (index == -1) {
			name = path;
			path = '';
		}
		else {
			name = path.substr(0, index);
			path = path.substr(index);
		}
		let actual = `${current}/${name}`;

		/* lookup the name in the parent */
		if (name in node.children)
			return this._getNode(node.children[name], actual, path, cb);

		/* check if the node itself is valid (i.e. exists in the read-only file-system) and is a directory */
		if (node.stats == null || node.stats.type != 'dir')
			return cb(null);

		/* setup the new node */
		let next = new FileNode(node, this._nodes.length, name);
		this._nodes.push(next);
		node.children[name] = next;

		/* request the stats (on errors, just pretend the object does not exist) */
		this._log(`Fetching stats for [${actual}]...`);
		fetch(`/stat${actual}`, { credentials: 'same-origin' })
			.then((resp) => {
				if (!resp.ok)
					throw new Error(`Failed to load [${actual}]`);
				return resp.json();
			})
			.then((json) => {
				this._log(`Stats for [${actual}] received`);
				if (json != null)
					next.setupStats(json, MemFileSystem.fsDefault);
			})
			.catch((err) => this._err(`Failed to fetch stats for [${actual}]: ${err}`))
			.finally(() => this._getNode(next, actual, path, cb));
	}
	_getValid(id) {
		if (id >= this._nodes.length || this._nodes[id] == null || this._nodes[id].stats == null)
			return null;
		return this._nodes[id];
	}
	_getNodePath(node) {
		let path = '';
		do {
			path = `/${node.name}${path}`;
		} while ((node = node.ancestor) != null && node.ancestor != null);
		return path;
	}
	_loadData(node, cb) {
		/* check if the data already exist */
		if (node.data != null)
			return cb();
		let path = this._getNodePath(node);

		/* fetch the existing file data */
		this._log(`Reading file [${path}]...`);
		fetch(`/data${path}`, { credentials: 'same-origin' })
			.then((resp) => {
				if (!resp.ok)
					throw new Error(`Failed to load [${path}]`);
				return resp.arrayBuffer();
			})
			.then((buf) => {
				this._log(`Data of [${path}] received`);
				node.data = buf;
				node.stats.size = node.data.byteLength;
			})
			.catch((err) => {
				this._err(`Failed to read data of [${path}]: ${err}`);

				/* pretend the file is empty */
				node.stats.size = 0;
				node.data = new ArrayBuffer();
			})
			.finally(() => cb());
	}

	/* cb(stats): stats either stats-object or null, if not found */
	getNode(path, cb) {
		this._getNode(this._root, '', path, (n) => cb(n == null ? null : n.stats));
	}

	/* cb(stats): fetch stats of node with given id (or null if does not exist) */
	getStats(id, cb) {
		let node = this._getValid(id);
		cb(node == null ? null : node.stats);
	}

	/* cb(path): fetch path of node with given id (or null if does not exist) */
	getPath(id, cb) {
		let node = this._getValid(id);
		if (node == null)
			return cb(null);
		return cb(this._getNodePath(node));
	}

	/* cb(success): mark file as read */
	setDirty(id, cb) {
		let node = this._getValid(id);
		if (node != null)
			node.read();
		return cb(node != null);
	}

	/* cb(success): true if succeeded, else false */
	fileResize(id, size, cb) {
		let node = this._getValid(id);
		if (node == null || node.stats.type != 'file')
			return cb(false);

		/* check if the size should be set to zero */
		if (size == 0) {
			if (node.stats.size != 0)
				node.written();
			node.stats.size = 0;
			node.data = null;
			return cb(true);
		}

		/* fetch the data of the file */
		this._loadData(node, () => {
			if (size == node.stats.size)
				return cb(true);

			/* check if the file should be reduced in size */
			if (size <= node.stats.size) {
				node.stats.size = size;
				node.data = node.data.slice(0, size);
				node.written();
				return cb(true);
			}

			/* increase the size of the buffer */
			try {
				let buf = new ArrayBuffer(size);
				new Uint8Array(buf).set(new Uint8Array(node.data), 0);
				node.stats.size = size;
				node.data = buf;
				node.written();
				return cb(true);
			} catch (e) {
				this._err(`Failed to allocate memory for node [${this._getNodePath(node)}]`);
				return cb(false);
			}
		});
	}

	/* cb(read): returns number of bytes read or null on failure */
	fileRead(id, buffer, offset, cb) {
		let node = this._getValid(id);
		if (node == null || node.stats.type != 'file')
			return cb(null);

		/* fetch the data of the node */
		this._loadData(node, () => {
			/* compute the number of bytes to read */
			let count = Math.min(buffer.byteLength, node.stats.size - offset);
			if (count <= 0)
				return cb(0);

			/* read the data to the buffer */
			buffer.set(new Uint8Array(node.data, offset, count));
			node.read();
			cb(count);
		});
	}

	/* cb(written): returns number of bytes written or null on failure */
	fileWrite(id, buffer, offset, cb) {
		let node = this._getValid(id);
		if (node == null || node.stats.type != 'file')
			return cb(null);

		/* fetch the data of the node */
		this._loadData(node, () => {
			/* compute the number of bytes to write */
			let count = Math.min(buffer.byteLength, node.stats.size - offset);
			if (count <= 0)
				return cb(0);

			/* write the data to the buffer */
			new Uint8Array(node.data, offset, count).set(buffer);
			node.written();
			cb(count);
		});
	}

	/* cb(id): id if succeeded, else null */
	fileCreate(id, name, owner, group, permissions, cb) {
		let node = this._getValid(id);
		if (node == null || node.stats.type != 'dir')
			return cb(null);

		/* check if the child already exists */
		this._getNode(node, this._getNodePath(node), name, (n) => {
			if (n.stats != null)
				return cb(null);

			/* setup the new file */
			n.setupEmptyStats('file', { owner: owner, group: group, file: permissions });
			return cb(n.id);
		});
	}
}
