/* import the main wasmlator system */
importScripts("./wasmlator.js")

/* handler to signal that wasmlator is not busy anymore */
let busy = true;
let completed = function () {
	busy = false;
	postMessage({ cmd: 'ready' });
};

/* handler to receive logs from wasmlator */
let logger = function (msg, failure) {
	postMessage({ cmd: 'log', msg: msg, failure: failure });
};

/* construct the actual wasmlator */
let wasmlator = setup_wasmlator(logger, completed);

/* handler to pass commands to the wasmlator environment */
onmessage = function (m) {
	if (busy)
		logger('Wasmlator.js is currently busy', true);
	else
		wasmlator(m.data, completed);
};
