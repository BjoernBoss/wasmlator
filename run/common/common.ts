export enum LogType {
	errInternal, logInternal, trace, debug, log, info, warning, error, fatal, output
};

export class FileStats {
	public id: number;
	public type: string;
	public link: string;
	public size: number;
	public atime_us: number;
	public mtime_us: number;
	public owner: number;
	public group: number;
	public permissions: number;

	public constructor(type: string) {
		this.link = '';
		this.size = 0;
		this.atime_us = Date.now() * 1000;
		this.mtime_us = this.atime_us;
		this.type = type;
		this.id = 0;
		this.owner = 0;
		this.group = 0;
		this.permissions = 0;
	}
}

export interface HostEnvironment {
	/* log information from wasmlator */
	log(type: LogType, msg: String): void;

	/* load the glue-module */
	loadGlue(imports: WebAssembly.Imports): Promise<WebAssembly.Instance>;

	/* load the main-module */
	loadMain(imports: WebAssembly.Imports): Promise<WebAssembly.Instance>;

	/* load any module */
	loadModule(imports: WebAssembly.Imports, buffer: ArrayBuffer): Promise<WebAssembly.Instance>;

	/* read user input */
	readInput(): Promise<string>;

	/* fetch the stats for the given path */
	fsLoadStats(path: string): Promise<FileStats | null>;

	/* fetch the data for the given path (of which the stats exist) */
	fsLoadData(path: string): Promise<Uint8Array>;
}
