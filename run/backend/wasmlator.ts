import { HostEnvironment, LogType } from './common';
import { FileSystem } from './filesystem';

class EmptyError extends Error { constructor(m: string) { super(m); this.name = ''; } };

class BusyResolver {
	private depth: number;
	private resolve: null | (() => void);

	public constructor() {
		this.depth = 0;
		this.resolve = null;
	}

	public start(): Promise<void> {
		if (this.depth != 0)
			throw new EmptyError('Wasmlator.js: Currently busy with another operation');
		this.depth = 1;
		return new Promise((res) => this.resolve = res);
	}
	public enter() {
		++this.depth;
	}
	public leave() {
		if (--this.depth > 0)
			return;
		let resolve = this.resolve!;
		this.resolve = null;
		resolve();
	}
};

class WasmLator {
	private busy: BusyResolver;
	private host: HostEnvironment;
	private fs: FileSystem;
	private glue: { memory: ArrayBuffer, exports: WebAssembly.Exports };
	private main: { memory: ArrayBuffer, exports: WebAssembly.Exports };

	public constructor(host: HostEnvironment) {
		this.host = host;
		this.fs = new FileSystem(host);
		this.busy = new BusyResolver();
		this.glue = { memory: new ArrayBuffer(0), exports: {} };
		this.main = { memory: new ArrayBuffer(0), exports: {} };
	}

	private logSelf(msg: string): void {
		this.host.log(LogType.logInternal, msg);
	}
	private errSelf(msg: string): void {
		this.host.log(LogType.errInternal, msg);
	}
	private logGuest(msg: string): void {
		let type = ((msg.length > 2 && msg[1] == ':') ? msg[0] : '_');

		switch (type) {
			case 'T':
				this.host.log(LogType.trace, msg.substring(2));
				break;
			case 'D':
				this.host.log(LogType.debug, msg.substring(2));
				break;
			case 'L':
				this.host.log(LogType.log, msg.substring(2));
				break;
			case 'I':
				this.host.log(LogType.info, msg.substring(2));
				break;
			case 'W':
				this.host.log(LogType.warning, msg.substring(2));
				break;
			case 'E':
				this.host.log(LogType.error, msg.substring(2));
				break;
			case 'F':
				this.host.log(LogType.fatal, msg.substring(2));
				break;
			default:
				this.host.log(LogType.fatal, `Malformed log-message encountered: ${msg}`);
				break;
		}
	}

	private loadString(ptr: number, size: number, fromMainMemory: boolean): string {
		let view = new DataView((fromMainMemory ? this.main.memory : this.glue.memory), ptr, size);
		return new TextDecoder('utf-8').decode(view);
	}
	private loadBuffer(ptr: number, size: number): ArrayBuffer {
		return this.main.memory.slice(ptr, ptr + size);
	};

	private buildGlueImports(): object {
		let _that = this;
		let imports: any = { host: {} };

		/* setup the glue imports */
		imports.host.host_get_export = function (instance: WebAssembly.Instance, ptr: number, size: number): WebAssembly.ExportValue | null {
			let obj = instance.exports[_that.loadString(ptr, size, true)];
			if (obj === undefined)
				return null;
			return obj;
		};
		imports.host.host_get_function = function (instance: WebAssembly.Instance, ptr: number, size: number, in_main: boolean): WebAssembly.ExportValue | null {
			let obj = instance.exports[_that.loadString(ptr, size, in_main)];
			if (obj === undefined)
				return null;
			return obj;
		};
		imports.host.host_make_object = function (): object { return {}; }
		imports.host.host_set_member = function (obj: Record<string, any>, name: number, size: number, value: any): void {
			obj[_that.loadString(name, size, true)] = value;
		}
		return imports;
	}
	private buildMainImports(): object {
		let _that = this;
		let imports: any = { host: {} };

		/* setup the main module imports */
		imports.env = { ...this.glue.exports };
		imports.env.emscripten_notify_memory_growth = function () {
			_that.main.memory = (_that.main.exports.memory as WebAssembly.Memory).buffer;
		};
		imports.wasi_snapshot_preview1.proc_exit = function (code: number): void {
			_that.errSelf(new EmptyError(`Main module terminated itself with exit-code [${code}] - (Unhandled exception?)`).stack!);
		};
		imports.env.host_task = function (ptr: number, size: number, process: number): void {
			_that.handleTask(_that.loadString(ptr, size, true), process);
		};
		imports.env.host_message = function (ptr: number, size: number): void {
			_that.logGuest(_that.loadString(ptr, size, true));
		};
		imports.env.host_failure = function (ptr: number, size: number): void {
			_that.logGuest(new EmptyError(_that.loadString(ptr, size, true)).stack!);
		};
		imports.env.host_random = function (): number { return Math.floor(Math.random() * 0x1_0000_0000); };
		imports.env.host_time_us = function (): BigInt { return BigInt(Date.now() * 1000); };
		imports.env.host_timezone_min = function (): number { return new Date().getTimezoneOffset(); };
		return imports;
	}

	private prepareTaskArgs(payload: string, n: number): [number[], string] {
		let split: string[] = payload.split(':');
		let args: number[] = [];
		let rest: string = '';
		for (let i = 0; i <= n; ++i) {
			if (i == n && i < split.length)
				rest = split.slice(n).join(':');
			else
				args.push(parseInt(split[i]));
		}
		return [args, rest];
	}
	private async handleTask(task: String, process: number) {
		/* enter the critical section - will be left by the task-completed callback */
		this.busy.enter();

		/* extract the next command */
		let cmd = task, i = 0, payload = '';
		if ((i = task.indexOf(':')) >= 0) {
			cmd = task.substring(0, i);
			payload = task.substring(i + 1);
		}

		/* handle the core and block creation handling */
		if (cmd == 'core') {
			let [args, _] = this.prepareTaskArgs(payload, 2);
			this.loadCore(this.loadBuffer(args[0], args[1]), process);
		}
		else if (cmd == 'block') {
			let [args, _] = this.prepareTaskArgs(payload, 2);
			this.loadBlock(this.loadBuffer(args[0], args[1]), process);
		}

		/* handle the file-system commands */
		else if (cmd.startsWith('resolve'))
			this.taskCompleted(process, await this.fs.getNode(payload));
		else if (cmd == 'stats')
			this.taskCompleted(process, await this.fs.getStats(parseInt(payload)));
		else if (cmd.startsWith('path'))
			this.taskCompleted(process, await this.fs.getPath(parseInt(payload)));
		else if (cmd == 'accessed')
			this.taskCompleted(process, await this.fs.setDirty(parseInt(payload)));
		else if (cmd == 'resize') {
			let [args, _] = this.prepareTaskArgs(payload, 2);
			this.taskCompleted(process, await this.fs.fileResize(args[0], args[1]));
		}
		else if (cmd == 'read') {
			let [args, _] = this.prepareTaskArgs(payload, 4);
			let buf = new Uint8Array(this.main.memory, args[1], args[3]);
			this.taskCompleted(process, await this.fs.fileRead(args[0], buf, args[2]));
		}
		else if (cmd == 'write') {
			let [args, _] = this.prepareTaskArgs(payload, 4);
			let buf = new Uint8Array(this.main.memory, args[1], args[3]);
			this.taskCompleted(process, await this.fs.fileWrite(args[0], buf, args[2]));
		}
		else if (cmd == 'create') {
			let [args, rest] = this.prepareTaskArgs(payload, 4);
			this.taskCompleted(process, await this.fs.fileCreate(args[0], rest, args[1], args[2], args[3]));
		}

		/* default catch-handler for unknown commands */
		else
			this.errSelf(new EmptyError(`Received unknown task [${cmd}]`).stack!);
	}
	private taskCompleted(process: number, payload?: any): void {
		/* write the result to the main application */
		let addr = 0, size = 0;
		if (payload != null) {
			let buffer = new TextEncoder().encode(JSON.stringify(payload));
			addr = (this.main.exports.main_allocate as (_: number) => number)(buffer.byteLength);
			size = buffer.byteLength;
			new Uint8Array(this.main.memory, addr, size).set(new Uint8Array(buffer));
		}

		/* invoke the callback and mark the critical section (opened by handle-task) as completed */
		try {
			(this.main.exports.main_task_completed as (_0: number, _1: number, _2: number) => void)(process, addr, size);
		}
		catch (err) {
			this.errSelf(`Failed to complete task: ${(err as Error).stack}`);
		}

		/* leave the critical section - was entered by handle-task */
		this.busy.leave();
	}

	private async loadCore(buffer: ArrayBuffer, process: number) {
		this.busy.enter();

		/* copy all glue/main exports as imports of the core (only the relevant will be bound) */
		let imports: any = {};
		imports.glue = { ...this.glue.exports };
		imports.main = { ...this.main.exports };

		try {
			/* load the module */
			let instance: WebAssembly.Instance = await this.host.loadModule(imports, buffer);

			/* set the last-instance, invoke the handler, and then reset the last-instance
			*	again, in order to ensure unused instances can be garbage-collected */
			(this.glue.exports.set_last_instance as (_?: WebAssembly.Instance) => void)(instance);
			this.taskCompleted(process);
			(this.glue.exports.set_last_instance as (_?: WebAssembly.Instance) => void)();
		} catch (err) {
			this.errSelf(`Failed to load core: ${err}`);
		}

		this.busy.leave();
	}
	private async loadBlock(buffer: ArrayBuffer, process: number) {
		this.busy.enter();

		/* fetch the imports-object */
		let imports: object = (this.glue.exports.get_imports as () => object)();

		try {
			/* load the module */
			let instance: WebAssembly.Instance = await this.host.loadModule(imports, buffer);

			/* set the last-instance, invoke the handler, and then reset the last-instance
			*	again, in order to ensure unused instances can be garbage-collected */
			(this.glue.exports.set_last_instance as (_?: WebAssembly.Instance) => void)(instance);
			this.taskCompleted(process);
			(this.glue.exports.set_last_instance as (_?: WebAssembly.Instance) => void)();
		} catch (err) {
			this.errSelf(`Failed to load block: ${err}`);
		}

		this.busy.leave();
	}

	public async prepareAndLoadMain(): Promise<boolean> {
		try {
			/* load the glue module and populate the state */
			this.logSelf('Loading glue module...');
			let glueInstance: WebAssembly.Instance = await this.host.loadGlue(this.buildGlueImports());
			this.logSelf('Glue module loaded');
			this.glue.exports = glueInstance.exports;
			this.glue.memory = (glueInstance.exports.memory as WebAssembly.Memory).buffer;
		}
		catch (err) {
			this.errSelf(`Failed to load glue module: ${err}`);
			return false;
		}

		try {
			/* load the main module and populate the state */
			this.logSelf('Loading main module...');
			let mainInstance: WebAssembly.Instance = await this.host.loadMain(this.buildMainImports());
			this.logSelf('Main module loaded');
			this.main.exports = mainInstance.exports;
			this.main.memory = (mainInstance.exports.memory as WebAssembly.Memory).buffer;
		}
		catch (err) {
			this.errSelf(`Failed to main glue module: ${err}`);
			return false;
		}

		try {
			let busy = this.busy.start();

			/* startup the main application (requires the internal _initialize to be invoked) */
			this.logSelf('Starting up main module...');
			(this.main.exports._initialize as () => void)();
			this.logSelf(`Main module initialized`);

			/* leave the busy section and await for it to complete (_initialize might trigger internal handlings) */
			this.busy.leave();
			await busy;
		}
		catch (err) {
			this.errSelf(`Failed to startup main application: ${(err as Error).stack}`);
			return false;
		}
		this.logSelf('Loaded successfully and ready....');
		return true;
	};
	public handle(msg: string): Promise<void> {
		let promise = this.busy.start();

		/* pass the actual command to the application */
		try {
			/* convert the string to a buffer */
			let buf = new TextEncoder().encode(msg);

			/* allocate a buffer for the string in the main-application and write the string to it */
			let ptr = (this.main.exports.main_allocate as (_0: number) => number)(buf.length);
			new Uint8Array(this.main.memory, ptr, buf.length).set(buf);

			/* perform the actual execution of the command (will ensure to free it) */
			(this.main.exports.main_user_command as (_0: number, _1: number) => void)(ptr, buf.length);
		} catch (err) {
			this.errSelf(`Failed to handle command: ${(err as Error).stack}`);
		}
		this.busy.leave();
		return promise;
	}
	public execute(msg: string): Promise<void> {
		let promise = this.busy.start();

		/* pass the payload to the application */
		try {
			/* convert the string to a buffer */
			let buf = new TextEncoder().encode(msg);

			/* allocate a buffer for the string in the main-application and write the string to it */
			let ptr = (this.main.exports.main_allocate as (_0: number) => number)(buf.length);
			new Uint8Array(this.main.memory, ptr, buf.length).set(buf);

			/* perform the actual execution of the command (will ensure to free it) */
			(this.main.exports.main_execute as (_0: number, _1: number) => void)(ptr, buf.length);
		} catch (err) {
			this.errSelf(`Failed to execute: ${(err as Error).stack}`);
		}
		this.busy.leave();
		return promise;
	}
};

class Interactable {
	private wasmlator: WasmLator;

	public constructor(wasmlator: WasmLator) {
		this.wasmlator = wasmlator;
	}

	public handle(msg: string): Promise<void> {
		return this.wasmlator.handle(msg);
	}
}

export async function SpawnInteract(host: HostEnvironment): Promise<Interactable | null> {
	let wasmlator: WasmLator = new WasmLator(host);
	if (!await wasmlator.prepareAndLoadMain())
		return null;
	return new Interactable(wasmlator);
}

export async function SpawnExecute(host: HostEnvironment, msg: string): Promise<void> {
	let wasmlator: WasmLator = new WasmLator(host);
	if (!await wasmlator.prepareAndLoadMain())
		return;
	return wasmlator.execute(msg);
}
