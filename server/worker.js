/* import the main wasmlator system */
importScripts("./wasmlator.js")

/* handler to signal that wasmlator is not busy anymore */
let completed = () => postMessage({ cmd: 'ready' });

/* handler to receive logs from wasmlator */
let logger = (msg, failure) => postMessage({ cmd: 'log', msg: msg, failure: failure });

/* construct the actual wasmlator */
let wasmlator = setup_wasmlator(logger, completed);

/* handler to pass commands to the wasmlator environment */
onmessage = function (m) {
	try {
		wasmlator(m.data, completed);
	} catch (e) {
		if (e instanceof BusyError)
			logger(e.toString(), true);
		else
			logger(`Unknown exception encountered: ${e.stack}`, true);
		completed();
	}
};
