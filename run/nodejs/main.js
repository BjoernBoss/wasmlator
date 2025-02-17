import { NodeHost } from './node-host.js';
import { SetupWasmlator } from '../../build/gen/wasmlator.js';
import { createInterface } from 'readline';

/* setup the handler to ensure killing via sig-int */
process.on('SIGINT', () => process.exit(1));

/* setup the io-reader, host, and load the wasmlator */
let reader = createInterface({ input: process.stdin, output: process.stdout });
let host = new NodeHost(reader, 'fs');
let wasmlator = await SetupWasmlator(host);

/* check if a single program should be executed */
if (process.argv.length > 2) {
	await wasmlator.execute(`"${process.argv.slice(2).join('" "')}"`);
	reader.close();
}

/* perform repeated executions in a loop */
else {
	let execute = function () {
		reader.question('> ', function (m) {
			wasmlator.execute(m).then(() => execute());
		});
	};
	execute();
}
