import { NodeHost } from './node-host.js';
import { SetupWasmlator } from '../generated/wasmlator.js';
import { createInterface } from 'readline';

process.chdir('run/nodejs');

let reader = createInterface({ input: process.stdin, output: process.stdout });
let host = new NodeHost(reader);
let wasmlator = await SetupWasmlator(host);

let execute = function () {
	reader.question('> ', function (m) {
		wasmlator.execute(m).then(() => execute());
	});
};

execute();
