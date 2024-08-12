
function testCall() {
	console.log('hi!');
	return 123;
}

mergeInto(LibraryManager.library, {
	testCall: testCall
});
