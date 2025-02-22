import { NodeHost } from './node-host.js';
import { SetupWasmlator } from '../../build/gen/wasmlator.js';
import { createInterface } from 'readline';
import { promises as fs } from 'fs';
import path from 'path';

/* fetch the root directory to be used */
let fsPath = (await fs.readFile('run/fs.root', { encoding: 'utf-8' })).trim();
try {
	if (!path.isAbsolute(fsPath))
		fsPath = await fs.realpath(path.join('./run', fsPath));
	else
		fsPath = await fs.realpath(fsPath);
	if (!(await fs.lstat(fsPath)).isDirectory())
		throw 1;
} catch (_) {
	console.error(`error: filesystem root [${fsPath}] is not a directory`);
	process.exit(1);
}

/* setup the io-reader, host, and load the wasmlator */
let reader = createInterface({ input: process.stdin, output: process.stdout });
let host = new NodeHost(reader, fsPath);
let wasmlator = await SetupWasmlator(host);

/* check if the application should be profiled */
let profiling = false;
if (process.argv.includes('--profile-wasmlator')) {
	process.argv = process.argv.filter((v) => v != '--profile-wasmlator');
	profiling = true;
}

/* define the profiling-printing function */
let printProfile = function (stats) {
	if (!profiling)
		return;
	let out = (v) => v.toFixed(3).padStart(12, ' ');

	console.log('\nMemory Usage:');
	console.log(`   FileSystem: ${out(stats.mem.filesystem / 1000)} Kb`);
	console.log(`   WasmLator : ${out(stats.mem.wasmlator / 1000)} Kb`);
	console.log(`   Guest     : ${out(stats.mem.guest / 1000)} Kb`);
	console.log(`   Peak Total: ${out(stats.mem.total / 1000)} Kb`);
	if (stats.mem.total >= 0) {
		let overhead = stats.mem.total - stats.mem.guest - stats.mem.wasmlator - stats.mem.filesystem
		console.log(`     Overhead: ${out(overhead / 1000)} Kb`);
	}
	console.log('\nTiming:');
	console.log(`  Startup    : ${out(stats.time.startup)} sec`);
	console.log(`  FileSystem : ${out(stats.time.filesystem)} sec`);
	console.log(`  Execution  : ${out(stats.time.executing)} sec`);
	console.log(`  Compilation: ${out(stats.time.compilation)} sec`);
	console.log(`  Total      : ${out(stats.time.total)} sec`);
	let misc = stats.time.total - stats.time.startup - stats.time.filesystem - stats.time.executing - stats.time.compilation;
	console.log(`    Misc     : ${out(misc)} sec`);
};

/* check if a single program should be executed */
if (process.argv.length > 2) {
	await wasmlator.execute(`"${process.argv.slice(2).join('" "')}"`);
	printProfile(wasmlator.stats());
	reader.close();
}

/* perform repeated executions in a loop */
else {
	let execute = function () {
		reader.question('> ', async function (m) {
			await wasmlator.execute(m);
			printProfile(wasmlator.stats());
			execute();
		});
	};
	execute();
}
