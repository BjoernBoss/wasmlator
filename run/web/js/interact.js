let handleEnteredCommand = function () { };

window.addEventListener('load', function () {
	let htmlOutput = document.getElementById('output');
	let htmlBusy = document.getElementById('busy');
	let buttonState = {
		'trace': true,
		'debug': true,
		'log': true,
		'info': true,
		'warn': true,
		'error': true,
		'fatal': true
	};

	/* register the button-behavior */
	for (const key in buttonState) {
		let html = document.getElementById(key);
		html.onclick = function () {
			buttonState[key] = html.classList.toggle('active');
			let style = (buttonState[key] ? 'block' : 'none');
			let list = htmlOutput.getElementsByClassName(key);
			for (let i = 0; i < list.length; ++i)
				list.item(i).style.display = style;
		};
	}

	/* register the clear button-behavior */
	document.getElementById('clear').onclick = function () {
		while (htmlOutput.children.length > 0)
			htmlOutput.children[0].remove();
	};

	/* logger function to write the logs to the ui */
	let logMessage = function (t, m) {
		/* classify the logging type to the ui style */
		let name = null;
		switch (t) {
			case 'logInternal':
				console.log(m);
				return;
			case 'errInternal':
				name = 'fatal';
				break;
			case 'trace':
			case 'debug':
			case 'log':
			case 'info':
			case 'warn':
			case 'error':
			case 'fatal':
				name = t;
				break;
			default:
				name = 'log';
				break;
		}

		/* check if its a failure and log it to the console */
		if (name == 'fatal')
			console.error(m);

		/* setup the new output entry */
		let e = document.createElement('div');
		htmlOutput.appendChild(e);

		/* patch the visibility */
		if (!buttonState[name])
			e.style.display = 'none';

		/* apply the style and write the text out */
		e.classList.add(name);
		e.innerText = m;
		e.scrollIntoView(true);
	};

	/* load the web-worker, which runs the wasmlator */
	let worker = new Worker('/worker.js', { type: 'module' });

	/* dispatch function to pass the next command to the worker */
	let inputQueue = [];
	let dispatchNext = function (cmd) {
		/* check if a command is being pushed to the queue */
		if (cmd != null) {
			inputQueue.push(cmd);

			/* check if another command is currently being handled */
			if (inputQueue.length > 1)
				return;
		}

		/* pop the last command from the queue and check if the queue is empty */
		else {
			inputQueue = inputQueue.splice(1);
			if (inputQueue.length == 0) {
				htmlBusy.style.visibility = 'hidden';
				return;
			}
		}

		/* dispatch the next command to the worker */
		htmlBusy.style.visibility = 'visible';
		worker.postMessage(inputQueue[0]);
	};

	/* register the command-handler to the worker */
	worker.onmessage = function (e) {
		let data = e.data;
		if (data.cmd == 'ready')
			dispatchNext(null);
		else if (data.cmd == 'log')
			logMessage(data.type, data.msg);
		else
			logMessage('errInternal', `Unknown command [${data.cmd}] received from service worker.`);
	};

	/* setup the command receiver function */
	handleEnteredCommand = function (m) {
		dispatchNext(m);
	};
});
