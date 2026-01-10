/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
/* import the main wasmlator system */
import { SetupWasmlator } from "/com/wasmlator.js";
import { LogType } from '/com/common.js';
import { WebHost } from '/js/web-host.js';

let inputQueue = [];
let inputPromise = null;
let wasmlator = null;
let isExecuteMode = false;
let isExecuting = false;

/* dispatch next input command accordingly */
let dispatchNext = function () {
	/* check if the queue is empty */
	if (inputQueue.length == 0) {
		if (inputPromise == null)
			postMessage({ cmd: 'ready' });
		return;
	}

	/* check if the input just needs to stay queued */
	if (wasmlator == null || (isExecuting && inputPromise == null))
		return;

	/* extract the next entry in the queue */
	let next = inputQueue[0];
	inputQueue = inputQueue.splice(1);

	/* check if input is expected */
	if (inputPromise != null) {
		inputPromise(next);
		return;
	}

	/* echo the input back */
	host.log(LogType.output, `> ${next}\n`);

	/* mark the execution as active */
	isExecuting = true;
	postMessage({ cmd: 'busy' });

	/* pass the command to the wasmlator */
	let promise = (isExecuteMode ? wasmlator.execute(next) : wasmlator.handle(next));
	promise.then(function () { })
		.catch((err) => host.log(LogType.errInternal, `Unknown exception while handling command: ${err.stack ?? err}`))
		.finally(function () {
			isExecuting = false;
			dispatchNext();
		});
};

/* construct the web-host */
let host = new WebHost((type, msg) => postMessage({ cmd: 'log', type: type, msg: msg }), function () {
	return new Promise(function (resolve) {
		host.log(LogType.logInternal, 'Awaiting input...');

		/* setup the input promise */
		inputPromise = function (input) {
			inputPromise = null;
			host.log(LogType.output, `${input}\n`);
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
	.catch((err) => host.log(LogType.errInternal, `Unknown exception while setting up wasmlator: ${err.stack ?? err}`))

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
		dispatchNext();
	}
};
