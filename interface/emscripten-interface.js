function jsLogImpl(ptr, size) {
	console.log(`Log: ${makeUTF8String(ptr, size)}`);
}
function jsFailImpl(ptr, size) {
	throw new Error(`Exception: ${makeUTF8String(ptr, size)}`);
}

mergeInto(LibraryManager.library, {
	jsLogImpl,
	jsFailImpl
});
