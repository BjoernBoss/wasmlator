window.addEventListener('load', function () {
	let htmlInput = document.getElementById('input');

	/* add the focus-forward handler when clicking anywhere on the content and fetch its initial focus */
	document.getElementById('content').onmouseup = function () {
		if (document.activeElement !== htmlInput && document.getSelection().toString().length == 0)
			htmlInput.focus();
	};
	htmlInput.focus();

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
			/* check if the value in the text box should be written to the volatile current history entry */
			if (inputDirty) {
				historyIndex = history.length - 1;
				history[historyIndex] = this.value;
				inputDirty = false;
			}

			/* check if the history is being traversed */
			if (e.code == 'ArrowUp' && historyIndex > 0)
				this.value = history[--historyIndex];
			else if (e.code == 'ArrowDown' && historyIndex < history.length - 1)
				this.value = history[++historyIndex];

			/* reset the committed-value to ensure modifications are detected */
			lastCommitted = this.value;
			return false;
		}

		/* check if a command has been entered, and the worker is currently not busy */
		if (e.code != 'Enter' && e.code != 'NumpadEnter')
			return true;

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

		/* pass the command to the handler */
		handleEnteredCommand(m);
		return false;
	}
});
