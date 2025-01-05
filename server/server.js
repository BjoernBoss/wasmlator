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
		'error': true
	};
	let history = ['', ''];
	let historyIndex = 0;
	let inputDirty = true;

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

	let busy = function () {
		htmlBusy.style.visibility = 'hidden';
	};
	let log = function (m, normal) {
		let e = document.createElement('div');

		let name = '';
		switch ((m.length <= 2 || !normal) ? 'F' : m[0]) {
			case 'T':
				name = 'trace';
				break;
			case 'D':
				name = 'debug';
				break;
			case 'L':
				name = 'log';
				break;
			case 'I':
				name = 'info';
				break;
			case 'W':
				name = 'warn';
				break;
			case 'E':
				name = 'error';
				break;
			default:
				break;
		}

		if (name != '') {
			m = m.substr(2);
			e.classList.add(name);
			if (!buttonState[name])
				e.style.display = 'none';
		}
		else
			e.classList.add('fatal');

		e.innerText = m;
		htmlOutput.appendChild(e);
		e.scrollIntoView(true);
	};

	let wasmlator = setup_wasmlator(busy, log);

	htmlInput.onkeydown = function (e) {
		if (e.code == 'ArrowUp' || e.code == 'ArrowDown') {
			if (inputDirty) {
				historyIndex = history.length - 1;
				history[historyIndex] = htmlInput.value;
				inputDirty = false;
			}

			if (e.code == 'ArrowUp' && historyIndex > 0)
				htmlInput.value = history[--historyIndex];
			else if (e.code == 'ArrowDown' && historyIndex < history.length - 1)
				htmlInput.value = history[++historyIndex];

			return false;
		}
		inputDirty = true;

		if (e.code != 'Enter' && e.code != 'NumpadEnter')
			return true;

		let m = htmlInput.value;
		htmlInput.value = '';

		let last = (history.length == 1 ? '' : history[history.length - 2]);
		if (m.length == 0) {
			m = last;
			history[history.length - 1] = '';
		}
		else if (m != last) {
			history[history.length - 1] = m;
			history.push('');
		}

		setTimeout(function () {
			try {
				htmlBusy.style.visibility = 'visible';
				wasmlator(m, busy);
			} catch (e) {
				log(e.stack, false);
			}
		});
		return false;
	}
}
