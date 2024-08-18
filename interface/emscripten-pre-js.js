function makeUTF8String(ptr, len) {
	let view = new DataView(Module.asm.memory.buffer, ptr, len);
	return new TextDecoder('utf-8').decode(view);
}
