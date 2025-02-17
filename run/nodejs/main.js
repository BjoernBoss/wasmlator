import { NodeHost } from './node-host.js';
import { SetupWasmlator } from '../generated/wasmlator.js';
import { createInterface } from 'readline';

/* ensure that the entire process is relative to the run directory */
process.chdir('run');

/* setup the io-reader, host, and load the wasmlator */
let reader = createInterface({ input: process.stdin, output: process.stdout });
let host = new NodeHost(reader, 'fs');
let wasmlator = await SetupWasmlator(host);

/* perform repeated executions in a loop */
let execute = function () {
	reader.question('> ', function (m) {
		wasmlator.execute(m).then(() => execute());
	});
};
execute();
