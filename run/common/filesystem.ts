import { HostEnvironment, FileStats, LogType } from './common.js';

class FileNode {
	public children: Record<string, FileNode>;
	public ancestor: FileNode | null;
	public stats: FileStats | null;
	public name: string;
	public data: Uint8Array | null;
	public id: number;

	public constructor(ancestor: FileNode | null, id: number, name: string) {
		this.children = {};
		this.ancestor = ancestor;
		this.stats = null;
		this.name = name;
		this.data = null;
		this.id = id;
	}

	public setupStats(stats: FileStats, access: Record<string, number>) {
		this.stats = stats;
		this.stats.id = this.id;
		this.stats.owner = access.owner;
		this.stats.group = access.group;
		this.stats.permissions = access[this.stats.type];
	}
	read() {
		this.stats!.atime_us = Date.now() * 1000;
	}
	written() {
		this.stats!.mtime_us = Date.now() * 1000;
	}
}

export class FileSystem {
	static readonly fsRoot: Record<string, number> = {
		owner: 0,
		group: 0,
		dir: 0o755
	};
	static readonly fsDefault: Record<string, number> = {
		owner: 1001,
		group: 1001,
		dir: 0o755,
		file: 0o754,
		link: 0o754
	};

	private host: HostEnvironment;
	private root: FileNode;
	private nodes: (FileNode | null)[];

	constructor(host: HostEnvironment) {
		this.host = host;

		/* setup the root node */
		this.root = new FileNode(null, 0, '');
		this.root.setupStats(new FileStats('dir'), FileSystem.fsRoot);

		/* map of all files */
		this.nodes = [this.root];
	}

	_getValid(id: number): FileNode | null {
		if (id >= this.nodes.length || this.nodes[id] == null || this.nodes[id].stats == null)
			return null;
		return this.nodes[id];
	}
	_getNodePath(node: FileNode): string {
		let path = '';
		do {
			path = `/${node.name}${path}`;
		} while ((node = node.ancestor!) != null && node.ancestor != null);
		return path;
	}
	async _getNode(node: FileNode, current: string, path: string): Promise<FileNode | null> {
		/* check if the end has been reached */
		if (path.length == 0 || path == '/')
			return node;

		/* strip the next part of the name and advance the path */
		if (path.startsWith('/'))
			path = path.substring(1);
		let index = path.indexOf('/'), name = '';
		if (index == -1) {
			name = path;
			path = '';
		}
		else {
			name = path.substring(0, index);
			path = path.substring(index);
		}
		let actual = `${current}/${name}`;

		/* lookup the name in the parent */
		if (name in node.children)
			return this._getNode(node.children[name], actual, path);

		/* check if the node itself is valid (i.e. exists in the read-only file-system) and is a directory */
		if (node.stats == null || node.stats.type != 'dir')
			return null;

		/* setup the new node */
		let next = new FileNode(node, this.nodes.length, name);
		this.nodes.push(next);
		node.children[name] = next;

		/* request the stats (on errors, just pretend the object does not exist) */
		this.host.log(LogType.logInternal, `Fetching stats for [${actual}]...`);
		try {
			let stats = await this.host.fsLoadStats(actual);
			this.host.log(LogType.logInternal, `Stats of [${actual}] received`);
			if (stats != null)
				next.setupStats(stats, FileSystem.fsDefault);
		}
		catch (err) {
			this.host.log(LogType.errInternal, `Failed to fetch stats for [${actual}]: ${err}`);
		}
		return this._getNode(next, actual, path);
	}
	async _loadData(node: FileNode): Promise<void> {
		/* check if the data already exist */
		if (node.data != null)
			return;
		let path = this._getNodePath(node);

		/* fetch the existing file data */
		this.host.log(LogType.logInternal, `Reading file [${path}]...`);
		try {
			let buf = await this.host.fsLoadData(path);
			this.host.log(LogType.logInternal, `Data of [${path}] received`);

			/* write the data to the node */
			node.data = buf;
			node.stats!.size = node.data.byteLength;
		}
		catch (err) {
			this.host.log(LogType.errInternal, `Failed to read data of [${path}]: ${err}`);

			/* pretend the file is empty */
			node.stats!.size = 0;
			node.data = new Uint8Array(0);
		}
	}

	async getNode(path: string): Promise<FileStats | null> {
		let node = await this._getNode(this.root, '', path);
		return (node == null ? null : node.stats);
	}
	async getStats(id: number): Promise<FileStats | null> {
		let node = this._getValid(id);
		return (node == null ? null : node.stats);
	}
	async getPath(id: number): Promise<string | null> {
		let node = this._getValid(id);
		if (node == null)
			return null;
		return this._getNodePath(node);
	}
	async setDirty(id: number): Promise<boolean> {
		let node = this._getValid(id);
		if (node != null)
			node.read();
		return (node != null);
	}
	async fileResize(id: number, size: number): Promise<boolean> {
		let node = this._getValid(id);
		if (node == null || node.stats!.type != 'file')
			return false;

		/* check if the size should be set to zero */
		if (size == 0) {
			if (node.stats!.size != 0)
				node.written();
			node.stats!.size = 0;
			node.data = new Uint8Array(0);
			return true;
		}

		/* fetch the data of the file and check if the given size already matches */
		await this._loadData(node);
		if (size == node.stats!.size)
			return true;

		/* check if the file should be reduced in size */
		if (size <= node.stats!.size) {
			node.stats!.size = size;
			node.data = node.data!.slice(0, size);
			node.written();
			return true;
		}

		/* increase the size of the buffer */
		try {
			let buf = new Uint8Array(size);
			buf.set(node.data!, 0);
			node.stats!.size = size;
			node.data = buf;
			node.written();
			return true;
		} catch (e) {
			this.host.log(LogType.errInternal, `Failed to allocate memory for node [${this._getNodePath(node)}]`);
			return false;
		}
	}
	async fileRead(id: number, buffer: Uint8Array, offset: number): Promise<number | null> {
		let node = this._getValid(id);
		if (node == null || node.stats!.type != 'file')
			return null;

		/* fetch the data of the node */
		await this._loadData(node);

		/* compute the number of bytes to read */
		let count = Math.min(buffer.byteLength, node.stats!.size - offset);
		if (count <= 0)
			return 0;

		/* read the data to the buffer */
		buffer.set(node.data!.slice(offset, offset + count));
		node.read();
		return count;
	}
	async fileWrite(id: number, buffer: Uint8Array, offset: number): Promise<number | null> {
		let node = this._getValid(id);
		if (node == null || node.stats!.type != 'file')
			return null;

		/* fetch the data of the node */
		await this._loadData(node);

		/* compute the number of bytes to write */
		let count = Math.min(buffer.byteLength, node.stats!.size - offset);
		if (count <= 0)
			return 0;

		/* write the data to the buffer */
		node.data!.slice(offset, offset + count).set(buffer);
		node.written();
		return count;
	}
	async fileCreate(id: number, name: string, owner: number, group: number, permissions: number): Promise<number | null> {
		let node = this._getValid(id);
		if (node == null || node.stats!.type != 'dir')
			return null;

		/* check if the child already exists (cannot return null as parent itself exists - provided that name does not contain a slash) */
		let next = (await this._getNode(node, this._getNodePath(node), name))!;
		if (next.stats != null)
			return null;

		/* setup the new file (including empty data to prevent loading non-existing file) */
		next.setupStats(new FileStats('file'), { owner: owner, group: group, file: permissions });
		node.stats!.size = 0;
		node.data = new Uint8Array(0);
		return next.id;
	}
}
