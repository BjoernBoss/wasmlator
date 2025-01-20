const fsDefaultUser = 1001;
const fsDefaultGroup = 1001;
const fsDirPermissions = 0o755;
const fsDefaultPermissions = 0o754;
const fsRootUser = 0;
const fsRootGroup = 0;
const fsRootPermissions = 0o755;

class FileNode {
	constructor(stats, ancestor, uniqueId) {
		this.ancestor = ancestor;
		this.dataDirty = false;
		this.metaDirty = false;
		this.stats = stats;
		this.users = 0;
		this.data = null;
		this.timeout = null;
		this.uniqueId = uniqueId;
		if (this.stats != null)
			this.stats.id = this.uniqueId;
	}

	setupEmptyStats(type) {
		this.stats = {};
		this.stats.link = '';
		this.stats.size = 0;
		this.stats.atime_us = Date.now() * 1000;
		this.stats.mtime_us = this.stats.atime_us;
		this.stats.type = type;
		this.stats.id = this.uniqueId;
	}
	setupAccess(owner, group, permissions) {
		this.stats.owner = owner;
		this.stats.group = group;
		this.stats.permissions = permissions;
	}
	ensureBuffer() {
		if (this.data == null)
			this.data = new ArrayBuffer(0, { maxByteLength: 40_000_000 });
	}
	addUser() {
		++this.users;

		/* check if a delete-timeout has been set and remove it */
		if (this.timeout != null)
			clearTimeout(this.timeout);
		this.timeout = null;
	}
	dropUser() {
		if (--this.users > 0 || this.dataDirty)
			return;

		/* check if the data should be held in memory or be discarded (delete files from memory after 1 minute of not being accessed) */
		this.timeout = setTimeout(() => {
			this.data = null;
		}, 60 * 1000);
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

class MemFileSystem {
	constructor(log, err) {
		this._log = log;
		this._err = err;
		this._nextId = 0;

		/* set of all open or queried files */
		this._files = {};

		/* map of ids to file nodes */
		this._open = [];
	}

	_getNode(path, cb) {
		/* check if the path has already been requested */
		if (path in this._files) {
			cb(this._files[path]);
			return;
		}

		/* check if the ancestor exists */
		new Promise((resolve) => {
			/* check if this is the root, in which case nothing needs to be done */
			if (path == '/')
				resolve(null);

			/* resolve the ancestor node */
			else {
				let index = path.lastIndexOf('/');
				this._getNode(index == 0 ? '/' : path.substr(0, index), (n) => resolve(n));
			}
		}).then((ancestor) => {
			if (path != '/') {
				/* check if the ancestor exists */
				if (ancestor == null || ancestor.stats == null) {
					cb(null);
					return;
				}

				/* check if the ancstor is a director */
				if (ancestor.stats.type != 'dir') {
					cb(null);
					return;
				}
			}

			/* request the stats */
			this._log(`Fetching stats for [${path}]...`);
			fetch(`/stat${path}`, { credentials: 'same-origin' })
				.then((resp) => {
					if (!resp.ok)
						throw new Error(`Failed to load [${path}]`);
					return resp.json();
				})
				.then((json) => {
					this._log(`Stats for [${path}] received`);
					this._files[path] = new FileNode(json, ancestor, ++this._nextId);
				})
				.catch((err) => {
					/* pretend the object does not exist */
					this._err(`Failed to fetch stats for [${path}]: ${err}`);
					this._files[path] = new FileNode(null, ancestor, ++this._nextId);
				})
				.finally(() => {
					let node = this._files[path];

					/* check if the node exists, and configure its access rights */
					if (node.stats != null)
						node.setupAccess(fsDefaultUser, fsDefaultGroup, (node.stats.type == 'dir' ? fsDirPermissions : fsDefaultPermissions));

					/* check if the node was not found, but is the root node, in which case an empty root is created */
					else if (path == '/') {
						node.setupEmptyStats('dir');
						node.setupAccess(fsRootUser, fsRootGroup, fsRootPermissions);
					}
					cb(node);
				});
		});
	}
	_openFile(node, path, create, open, truncate, owner, group, permissions, cb) {
		/* validate already existing objects */
		if (node.stats != null) {
			/* check if the type is valid */
			if (node.stats.type != 'file') {
				cb(null, node.stats);
				return;
			}

			/* check if an existing file can be opened */
			if (!open) {
				cb(null, node.stats);
				return;
			}
		}

		/* check if the file can be created */
		else if (!create) {
			cb(null, node.stats);
			return;
		}

		/* add the user and allocate the open-file index */
		node.addUser();
		let openId = this._open.length;
		this._open.push(node);


		/* check if the file should be truncated or is being newly created */
		if (truncate || node.stats == null) {
			node.ensureBuffer();

			/* setup the new file and its access permissions and mark the ancstor as written (parent-directory) */
			if (node.stats == null) {
				node.setupEmptyStats('file');
				node.setupAccess(owner, group, permissions);
				node.ancestor.written();
			}

			/* truncate the existing file */
			if (node.data.byteLength > 0 || node.stats.size > 0) {
				node.stats.size = 0;
				node.data.resize(0);
				node.written();
			}

			/* notify the callback about the opened file */
			cb(openId, node.stats);
			return;
		}

		/* check if the file data have already been fetched */
		if (node.data != null) {
			cb(openId, node.stats);
			return;
		}
		node.ensureBuffer();

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
				buf = new Uint8Array(buf);

				/* write the data to the node */
				node.stats.size = buf.byteLength;
				node.data.resize(buf.byteLength);
				new Uint8Array(node.data).set(buf);
			})
			.catch((err) => {
				this._err(`Failed to read data of [${path}]: ${err}`);

				/* pretend the file has been cleared (i.e. all data have been deleted by another user) */
				node.stats.size = 0;
				node.data.resize(0);
			})
			.finally(() => cb(openId, node.stats));
	}

	/* cb(stats): stats either stats-object or null, if not found */
	getStats(path, cb) {
		this._getNode(path, (n) => cb(n == null ? null : n.stats));
	}

	/* cb(id, stats): id if succedded, else null, stats if exists, else null */
	openFile(path, create, open, truncate, owner, group, permissions, cb) {
		/* fetch the stats of the file */
		this._getNode(path, (node) => {
			/* check if the node does not exist (i.e. ancestor does not exist) */
			if (node == null)
				cb(null, null);

			/* open or create the new file */
			else
				this._openFile(node, path, create, open, truncate, owner, group, permissions, cb);
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

	/* cb(): close the file and simply return */
	closeFile(id, cb) {
		/* validate the id and simply discard any operations on
		*	unknown ids and otherwise remove the user */
		if (id < this._open.length && this._open[id] != null) {
			this._open[id].dropUser();
			this._open[id] = null;
		}

		/* notify the listener about the completion */
		cb();
	}
}
