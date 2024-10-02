
function memoryImports(imports) {
	imports['memory'] = {};
	imports['memory'].import_lookup = _env.self.asm.MemoryPerformLookup;
	imports['memory'].mem_import_lookup_offset = _env.self.asm.MemoryLastLookupOffset;
	imports['memory'].mem_import_lookup_size = _env.self.asm.MemoryLastLookupSize;
}
