let handleEnteredCommand = function () { };

window.addEventListener('load', function () {
	let htmlOutput = document.getElementById('output');
	let htmlContent = document.getElementById('content');
	let htmlPrompt = this.document.getElementById('prompt');

	/* register the clear button-behavior */
	let lastCanAppend = false;
	document.getElementById('clear').onclick = function () {
		while (htmlOutput.children.length > 0)
			htmlOutput.children[0].remove();
		lastCanAppend = false;
	};

	/* logger function to write the logs to the ui */
	let logMessage = function (t, m) {
		if (m.length == 0)
			return;

		/* classify the logging type to the ui style */
		let fatal = false;
		switch (t) {
			case 'logInternal':
				console.log(m);
				return;
			case 'trace':
			case 'debug':
			case 'log':
			case 'info':
			case 'warn':
			case 'error':
			case 'output':
				fatal = false;
				break;
			case 'errInternal':
			case 'fatal':
			default:
				console.error(m);
				fatal = true;
				break;
		}

		/* check if the child should be scolled into view */
		let scrollIntoView = (htmlContent.scrollHeight - htmlContent.scrollTop <= htmlContent.clientHeight + 100);

		/* check if the last entry and this entry are both normal outputs, in which case they can be appended */
		let e = null;
		if (lastCanAppend && t == 'output')
			e = htmlOutput.lastChild;
		else {
			/* setup the new output entry */
			e = document.createElement('div');
			htmlOutput.appendChild(e);

			/* patch the style */
			if (fatal)
				e.classList.add('fatal');
		}

		/* update the append attribute (only if the last entry was an output and did not end on a linebreak) */
		lastCanAppend = (t == 'output');
		if (lastCanAppend && m.endsWith('\n')) {
			lastCanAppend = false;
			m = m.substring(0, m.length - 1);
		}

		/* write the text out and scroll it into view */
		e.innerText += m;
		if (scrollIntoView)
			e.scrollIntoView(true);
	};

	/* load the web-worker, which runs the wasmlator */
	let worker = new Worker('/js/worker.js', { type: 'module' });

	/* register the command-handler to the worker */
	worker.onmessage = function (e) {
		let data = e.data;
		if (data.cmd == 'ready')
			htmlPrompt.style.display = 'block';
		else if (data.cmd == 'busy')
			htmlPrompt.style.display = 'none';
		else if (data.cmd == 'log')
			logMessage(data.type, data.msg);
		else
			logMessage('errInternal', `Unknown command [${data.cmd}] received from service worker.`);
	};

	/* setup the command receiver function */
	handleEnteredCommand = (m) => worker.postMessage({ cmd: m, execute: true });
});
