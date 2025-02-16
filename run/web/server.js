window.onload = function () {
	let htmlOutput = document.getElementById('output');
	let htmlInput = document.getElementById('input');
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

	/* add the focus-forward handler when clicking anywhere on the content */
	document.getElementById('content').onmouseup = function () {
		if (document.activeElement !== htmlInput && document.getSelection().toString().length == 0)
			htmlInput.focus();
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
	let workerBusy = true;

	/* register the command-handler to the worker */
	worker.onmessage = function (e) {
		let data = e.data;
		if (data.cmd == 'ready') {
			htmlBusy.style.visibility = 'hidden';
			workerBusy = false;
		}
		else if (data.cmd == 'log')
			logMessage(data.type, data.msg);
		else
			logMessage('errInternal', `Unknown command [${data.cmd}] received from service worker.`);
	};

	/* register the handler for creating the history behavior and committing the commands */
	let history = ['', ''];
	let historyIndex = 0;
	let inputDirty = true;
	let lastCommitted = '';
	htmlInput.onkeydown = function (e) {
		this.scrollIntoView(true);

		/* check if the input is considered dirty */
		if (this.value != lastCommitted)
			inputDirty = true;

		/* check if the history should be looked at */
		if (e.code == 'ArrowUp' || e.code == 'ArrowDown') {
			if (inputDirty) {
				historyIndex = history.length - 1;
				history[historyIndex] = this.value;
				inputDirty = false;
			}

			if (e.code == 'ArrowUp' && historyIndex > 0)
				this.value = history[--historyIndex];
			else if (e.code == 'ArrowDown' && historyIndex < history.length - 1)
				this.value = history[++historyIndex];
			lastCommitted = this.value;
			return false;
		}

		/* check if a command has been entered, and the worker is currently not busy */
		if (e.code != 'Enter' && e.code != 'NumpadEnter')
			return true;
		if (workerBusy)
			return false;

		/* extract the command */
		let m = this.value;
		this.value = '';

		/* update the history */
		let last = (history.length == 1 ? '' : history[history.length - 2]);
		if (m.length == 0) {
			m = last;
			history[history.length - 1] = '';
		}
		else if (m != last) {
			history[history.length - 1] = m;
			history.push('');
		}

		/* reset the input state */
		lastCommitted = '';
		inputDirty = false;
		historyIndex = history.length - 1;

		/* send the command to the worker and mark him as busy */
		htmlBusy.style.visibility = 'visible';
		workerBusy = true;
		worker.postMessage(m);
		return false;
	}
}
