/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
let handleEnteredCommand = function () { };

window.addEventListener('load', function () {
	let htmlOutput = document.getElementById('output');
	let htmlContent = document.getElementById('content');
	let htmlBusy = document.getElementById('busy');
	let htmlPrompt = this.document.getElementById('prompt');
	let buttonState = {
		'trace': 0,
		'debug': 0,
		'log': 2,
		'info': 2,
		'warn': 2,
		'error': 2,
		'fatal': 2
	};
	let styleList = ['disabled', null, 'active'];

	/* register the button-behavior and initialize the state */
	for (const key in buttonState) {
		let html = document.getElementById(key);
		if (buttonState[key] != 1)
			html.classList.add(styleList[buttonState[key]]);

		/* add the toggle behavior */
		html.onclick = function () {
			if (buttonState[key] != 1)
				html.classList.remove(styleList[buttonState[key]]);
			buttonState[key] = (buttonState[key] == 0 ? 2 : buttonState[key] - 1);
			if (buttonState[key] != 1)
				html.classList.add(styleList[buttonState[key]]);

			/* show/hide all affected log entries */
			let style = (buttonState[key] == 2 ? 'block' : 'none');
			let list = htmlOutput.getElementsByClassName(key);
			for (let i = 0; i < list.length; ++i)
				list.item(i).style.display = style;
		};
	}

	/* register the clear button-behavior */
	let lastCanAppend = false;
	document.getElementById('clear').onclick = function () {
		htmlOutput.replaceChildren();
		lastCanAppend = false;
	};

	/* logger function to write the logs to the ui */
	let logMessage = function (t, m) {
		if (m.length == 0)
			m = ' ';

		/* classify the logging type to the ui style */
		let name = null;
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
				name = t;
				break;
			case 'output':
				name = 'log';
				break;
			case 'errInternal':
			case 'fatal':
			default:
				name = 'fatal';
				console.error(m);
				break;
		}

		/* check if the message should be discarded */
		if (buttonState[name] == 0)
			return;

		/* check if the child should be scolled into view */
		let scrollIntoView = (htmlContent.scrollHeight - htmlContent.scrollTop <= htmlContent.clientHeight + 100);

		/* check if the last entry and this entry are both normal outputs, in which case they can be appended */
		let e = null;
		if (lastCanAppend && t == 'output')
			e = htmlOutput.lastChild;
		else {
			/* setup the new output entry */
			e = document.createElement('div');
			e.innerText = '';
			htmlOutput.appendChild(e);

			/* patch the visibility and the style */
			if (buttonState[name] != 2)
				e.style.display = 'none';
			e.classList.add(name);
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

	/* load the web-worker, which runs the wasmlator, and configure it accordingly */
	let worker = new Worker('/js/worker.js', { type: 'module' });
	worker.postMessage('handle');

	/* register the command-handler to the worker */
	worker.onmessage = function (e) {
		let data = e.data;
		if (data.cmd == 'ready') {
			htmlBusy.style.visibility = 'hidden';
			htmlPrompt.style.display = 'block';
		}
		else if (data.cmd == 'busy') {
			htmlBusy.style.visibility = 'visible';
			htmlPrompt.style.display = 'none';
		}
		else if (data.cmd == 'log')
			logMessage(data.type, data.msg);
		else
			logMessage('errInternal', `Unknown command [${data.cmd}] received from service worker.`);
	};

	/* setup the command receiver function */
	handleEnteredCommand = (m) => worker.postMessage(m);
});
