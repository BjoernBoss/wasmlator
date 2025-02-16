import { FileStats, HostEnvironment, LogType } from './common.js';

export class WebHost implements HostEnvironment {
	private logger: (type: string, msg: string) => void;
	private userInput: () => Promise<string>;

	public constructor(logger: (type: string, msg: string) => void, userInput: () => Promise<string>) {
		this.logger = logger;
		this.userInput = userInput;
	}

	log(type: LogType, msg: string): void {
		this.logger(LogType[type], msg);
	}
	async loadGlue(imports: WebAssembly.Imports): Promise<WebAssembly.Instance> {
		let response = await fetch('/gen/glue.wasm', { credentials: 'same-origin' });
		let instantiated = await WebAssembly.instantiateStreaming(response, imports);
		return instantiated.instance;
	}

	async loadMain(imports: WebAssembly.Imports): Promise<WebAssembly.Instance> {
		let response = await fetch('/gen/main.wasm', { credentials: 'same-origin' });
		let instantiated = await WebAssembly.instantiateStreaming(response, imports);
		return instantiated.instance;
	}
	async loadModule(imports: WebAssembly.Imports, buffer: ArrayBuffer): Promise<WebAssembly.Instance> {
		let instantiated = await WebAssembly.instantiate(buffer, imports);
		return instantiated.instance;
	}
	async readInput(): Promise<string> {
		return this.userInput();
	}
	async fsLoadStats(path: string): Promise<FileStats | null> {
		let response = await fetch(`/stat${path}`, { credentials: 'same-origin' });
		let json = await response.json();
		if (json == null)
			return null;

		/* populate the filestats with the values received from the remote */
		let stats = new FileStats(json.type as string);
		stats.link = (json.link as string);
		stats.size = (json.size as number);
		stats.atime_us = (json.atime_us as number);
		stats.mtime_us = (json.mtime_us as number);
		return stats;
	}
	async fsLoadData(path: string): Promise<ArrayBuffer> {
		let response = await fetch(`/data${path}`, { credentials: 'same-origin' });
		return response.arrayBuffer();
	}
}
