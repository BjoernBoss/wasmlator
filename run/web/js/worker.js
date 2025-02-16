/* import the main wasmlator system */
import { SetupWasmlator } from "/gen/wasmlator.js";
import { LogType } from '/gen/common.js';
import { WebHost } from '/gen/web-host.js';

/* construct the actual wasmlator (never post ready on failed construction - i.e. do not recover from it) */
let host = new WebHost((type, msg) => postMessage({ cmd: 'log', type: type, msg: msg }));
let wasmlator = null;
SetupWasmlator(host)
	.then(function (v) {
		wasmlator = v;
		dispatchNext();
	})
	.catch((err) => host.log(LogType.errInternal, `Unknown exception while setting up wasmlator: ${err.stack}`))

/* function to dispatch the next command */
let inputQueue = [];
let dispatchNext = function () {
	/* check if the queue is empty */
	if (inputQueue.length == 0) {
		postMessage({ cmd: 'ready' });
		return;
	}

	/* pass the command to the wasmlator */
	let cmd = inputQueue[0].cmd;
	let promise = (inputQueue[0].execute ? wasmlator.execute(cmd) : wasmlator.handle(cmd));
	promise.then(function () { })
		.catch((err) => host.log(LogType.errInternal, `Unknown exception while handling command: ${err.stack}`))
		.finally(function () {
			inputQueue = inputQueue.splice(1);
			dispatchNext();
		});
};

/* setup the input-queue and command receiver */
onmessage = function (e) {
	let data = e.data;
	inputQueue.push({ cmd: data.cmd, execute: data.execute });
	if (inputQueue.length == 1) {
		postMessage({ cmd: 'busy' });
		dispatchNext();
	}
}
