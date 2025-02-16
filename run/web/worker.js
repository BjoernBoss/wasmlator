/* import the main wasmlator system */
import { SpawnInteract } from "/gen/wasmlator.js";
import { LogType } from '/gen/common.js';
import { WebHost } from '/gen/web-host.js';

/* handler to signal that wasmlator is not busy anymore */
let completed = () => postMessage({ cmd: 'ready' });

/* handler to receive logs from wasmlator */
let logger = (type, msg) => postMessage({ cmd: 'log', type: type, msg: msg });

/* construct the actual wasmlator */
let host = new WebHost(logger);
let wasmlator = null;
SpawnInteract(host)
	.then(function (v) { wasmlator = v; completed(); })
	.catch((err) => host.log(LogType.errInternal, `Unknown exception while setting up wasmlator: ${err.stack}`))

/* handler to pass commands to the wasmlator environment */
onmessage = function (m) {
	wasmlator.handle(m.data)
		.then(function () { })
		.catch((err) => host.log(LogType.errInternal, `Unknown exception while handling command: ${err.stack}`))
		.finally(() => completed());
};
