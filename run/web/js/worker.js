/* import the main wasmlator system */
import { SetupWasmlator } from "/gen/wasmlator.js";
import { LogType } from '/gen/common.js';
import { WebHost } from '/gen/web-host.js';

let inputQueue = [];
let inputPromise = null;
let wasmlator = null;
let isExecuteMode = false;

/* dispatch next input command accordingly */
let dispatchNext = function () {
	/* check if the queue is empty */
	if (inputQueue.length == 0) {
		if (inputPromise == null)
			postMessage({ cmd: 'ready' });
		return;
	}

	/* extract the next entry in the queue */
	let next = inputQueue[0];
	inputQueue = inputQueue.splice(1);

	/* check if input is expected */
	if (inputPromise != null) {
		inputPromise(next);
		return;
	}

	/* pass the command to the wasmlator */
	let promise = (isExecuteMode ? wasmlator.execute(next) : wasmlator.handle(next));
	promise.then(function () { })
		.catch((err) => host.log(LogType.errInternal, `Unknown exception while handling command: ${err.stack}`))
		.finally(() => dispatchNext());
};

/* construct the web-host */
let host = new WebHost((type, msg) => postMessage({ cmd: 'log', type: type, msg: msg }), function () {
	return new Promise(function (resolve) {
		host.log(LogType.logInternal, 'Awaiting input...');
		inputPromise = function (input) {
			inputPromise = null;
			resolve(input);
		};
		dispatchNext();
	});
});

/* construct the actual wasmlator (never post ready on failed construction - i.e. do not recover from it) */
SetupWasmlator(host)
	.then(function (v) {
		wasmlator = v;
		dispatchNext();
	})
	.catch((err) => host.log(LogType.errInternal, `Unknown exception while setting up wasmlator: ${err.stack}`))

/* setup the initial input receiver to receive the initial configuration */
onmessage = function (e) {
	if (e.data == 'execute')
		isExecuteMode = true;
	else if (e.data != 'handle') {
		host.log(LogType.errInternal, `Unknown configuration: ${e.data}`);
		return;
	}

	/* setup the actual input receiver */
	onmessage = function (e) {
		inputQueue.push(e.data);
		if (inputQueue.length == 1) {
			postMessage({ cmd: 'busy' });
			dispatchNext();
		}
	}
};
