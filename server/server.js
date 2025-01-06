window.onload = function () {
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

	/* logger function to write the logs to the ui */
	let logMessage = function (m, failure) {
		if (failure)
			console.error(m);
		let e = document.createElement('div');
		htmlOutput.appendChild(e);

		/* lookup the log-type */
		let name = {
			'T:': 'trace',
			'D:': 'debug',
			'L:': 'log',
			'I:': 'info',
			'W:': 'warn',
			'E:': 'error'
		}[m.substr(0, 2)];

		/* update the string and patch the types and visibility */
		if (name !== undefined)
			m = m.substr(2);
		else
			name = (failure ? 'fatal' : 'log');
		if (!buttonState[name])
			e.style.display = 'none';

		/* apply the style and write the text out */
		e.classList.add(name);
		e.innerText = m;
		e.scrollIntoView(true);
	};

	/* load the web-worker, which runs the wasmlator */
	let worker = new Worker('./worker.js');
	let workerBusy = true;

	/* register the command-handler to the worker */
	worker.onmessage = function (e) {
		let data = e.data;
		if (data.cmd == 'ready') {
			htmlBusy.style.visibility = 'hidden';
			workerBusy = false;
		}
		else if (data.cmd == 'log')
			logMessage(data.msg, data.failure);
		else
			logMessage(`Unknown command [${data.cmd}] received from service worker.`, true);
	};

	/* register the handler for creating the history behavior and committing the commands */
	let history = ['', ''];
	let historyIndex = 0;
	let inputDirty = true;
	document.getElementById('input').onkeydown = function (e) {
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

			return false;
		}
		inputDirty = true;

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

		/* send the command to the worker and mark him as busy */
		htmlBusy.style.visibility = 'visible';
		workerBusy = true;
		worker.postMessage(m);
		return false;
	}
}
