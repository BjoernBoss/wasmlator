# help-menu
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "TARGETS:"
	@echo "   help (print this menu; default)"
	@echo "   clean (remove all build outputs)"
	@echo "   server (generate the glue module and main module to server/wasm)"
	@echo "   wat (generate example glue/core/block modules to server/wat)"
	@echo "   wasm (generate example glue/core/block modules to server/wasm)"

# build-output directory
path_build := build/make

# server-output directory
path_server := server

# default emscripten compiler with all relevant flags
em := em++ -std=c++20 -I./repos -O1
em_main := $(em) --no-entry -sERROR_ON_UNDEFINED_SYMBOLS=0 -sWARN_ON_UNDEFINED_SYMBOLS=0 -sWASM_BIGINT -sALLOW_MEMORY_GROWTH -sSTANDALONE_WASM\
 -sEXPORTED_FUNCTIONS=_main_startup,_main_core_loaded,_main_set_exit_code,_main_resolve,_main_flushed,_main_block_loaded,_main_lookup,_main_result_address,_main_result_physical,_main_result_size

# default clang-compiler with all relevant flags
cc := clang++ -std=c++20 -O3 -I./repos

# repos/ustring includes
ustring_includes := $(wildcard repos/ustring/*.h) $(wildcard repos/ustring/*/*.h) $(wildcard repos/ustring/*/*/*.h)

# repos/wasgen compilation
_out_wasgen := $(path_build)/wasgen
$(_out_wasgen):
	mkdir -p $(_out_wasgen)
wasgen_objects := $(patsubst %,$(_out_wasgen)/%.o,module sink target binary-base binary-module binary-sink text-base text-module text-sink)
wasgen_objects_cc := $(wasgen_objects)
wasgen_objects_em := $(subst .o,.em.o,$(wasgen_objects))
wasgen_includes := $(wildcard repos/wasgen/*.h) $(wildcard repos/wasgen/*/*.h) $(wildcard repos/wasgen/*/*/*.h)
_wasgen_prerequisites := $(wasgen_includes) $(ustring_includes) | $(_out_wasgen)
$(_out_wasgen)/module.o: repos/wasgen/objects/wasm-module.cpp $(_wasgen_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_wasgen)/sink.o: repos/wasgen/sink/wasm-sink.cpp $(_wasgen_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_wasgen)/target.o: repos/wasgen/sink/wasm-target.cpp $(_wasgen_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_wasgen)/binary-base.o: repos/wasgen/writer/binary/binary-base.cpp $(_wasgen_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_wasgen)/binary-module.o: repos/wasgen/writer/binary/binary-module.cpp $(_wasgen_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_wasgen)/binary-sink.o: repos/wasgen/writer/binary/binary-sink.cpp $(_wasgen_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_wasgen)/text-base.o: repos/wasgen/writer/text/text-base.cpp $(_wasgen_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_wasgen)/text-module.o: repos/wasgen/writer/text/text-module.cpp $(_wasgen_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_wasgen)/text-sink.o: repos/wasgen/writer/text/text-sink.cpp $(_wasgen_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@

# self compilation
_out_self := $(path_build)/self
$(_out_self):
	mkdir -p $(_out_self)
self_objects := $(patsubst %,$(_out_self)/%.o,context-access context-bridge env-context env-mapping mapping-access mapping-bridge\
				env-memory memory-access memory-bridge memory-mapper env-process process-access process-bridge interface\
				host address-writer trans-address context-builder context-writer mapping-builder mapping-writer memory-builder\
				memory-writer trans-superblock trans-writer trans-core glue-state trans-glue trans-translator)
self_objects_cc := $(self_objects)
self_objects_em := $(subst .o,.em.o,$(self_objects))
_self_prerequisites := $(ustring_includes) $(wasgen_includes) $(wildcard env/*.h) $(wildcard env/*/*.h) $(wildcard trans/*.h) $(wildcard trans/*/*.h) $(wildcard interface/*.h) | $(_out_self)
$(_out_self)/context-access.o: env/context/context-access.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/context-bridge.o: env/context/context-bridge.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/env-context.o: env/context/env-context.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/mapping-access.o: env/mapping/mapping-access.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/mapping-bridge.o: env/mapping/mapping-bridge.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/env-mapping.o: env/mapping/env-mapping.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/env-memory.o: env/memory/env-memory.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/memory-access.o: env/memory/memory-access.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/memory-bridge.o: env/memory/memory-bridge.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/memory-mapper.o: env/memory/memory-mapper.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/env-process.o: env/env-process.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/process-access.o: env/process/process-access.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/process-bridge.o: env/process/process-bridge.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/interface.o: interface/interface.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/host.o: interface/host.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/address-writer.o: trans/address/address-writer.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/trans-address.o: trans/address/trans-address.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/context-builder.o: trans/context/context-builder.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/context-writer.o: trans/context/context-writer.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/mapping-builder.o: trans/mapping/mapping-builder.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/mapping-writer.o: trans/mapping/mapping-writer.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/memory-builder.o: trans/memory/memory-builder.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/memory-writer.o: trans/memory/memory-writer.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/trans-superblock.o: trans/translator/trans-superblock.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/trans-writer.o: trans/translator/trans-writer.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/trans-core.o: trans/core/trans-core.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/glue-state.o: trans/glue/glue-state.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/trans-glue.o: trans/glue/trans-glue.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@
$(_out_self)/trans-translator.o: trans/trans-translator.cpp $(_self_prerequisites)
	$(em) -c $< -o $(subst .o,.em.o,$@)
	$(cc) -c $< -o $@

# make-glue.exe compilation
glue_path := $(path_build)/make-glue.exe
$(glue_path): entry/make-glue.cpp entry/null-interface.cpp $(self_objects) $(wasgen_objects)
	$(cc) entry/make-glue.cpp entry/null-interface.cpp $(self_objects_cc) $(wasgen_objects_cc) -o $@

# make-block.exe compilation
block_path := $(path_build)/make-block.exe
$(block_path): entry/make-block.cpp entry/null-interface.cpp $(self_objects) $(wasgen_objects)
	$(cc) entry/make-block.cpp entry/null-interface.cpp $(self_objects_cc) $(wasgen_objects_cc) -o $@

# make-core.exe compilation
core_path := $(path_build)/make-core.exe
$(core_path): entry/make-core.cpp entry/null-interface.cpp $(self_objects) $(wasgen_objects)
	$(cc) entry/make-core.cpp entry/null-interface.cpp $(self_objects_cc) $(wasgen_objects_cc) -o $@

# generate all wat output
_out_wat := $(path_server)/wat
$(_out_wat):
	mkdir -p $(_out_wat)
$(_out_wat)/glue-module.wat: $(glue_path) | $(_out_wat)
	$(glue_path) $(_out_wat)/glue-module.wat
$(_out_wat)/core-module.wat: $(core_path) | $(_out_wat)
	$(core_path) $(_out_wat)/core-module.wat
$(_out_wat)/block-module.wat: $(block_path) | $(_out_wat)
	$(block_path) $(_out_wat)/block-module.wat
wat: $(_out_wat)/glue-module.wat $(_out_wat)/core-module.wat $(_out_wat)/block-module.wat
.PHONY: wat

# generate all wasm output
_out_wasm := $(path_server)/wasm
$(_out_wasm):
	mkdir -p $(_out_wasm)
$(_out_wasm)/glue-module.wasm: $(glue_path) | $(_out_wasm)
	$(glue_path) $(_out_wasm)/glue-module.wasm
$(_out_wasm)/core-module.wasm: $(core_path) | $(_out_wasm)
	$(core_path) $(_out_wasm)/core-module.wasm
$(_out_wasm)/block-module.wasm: $(block_path) | $(_out_wasm)
	$(block_path) $(_out_wasm)/block-module.wasm
wasm: $(_out_wasm)/glue-module.wasm $(_out_wasm)/core-module.wasm $(_out_wasm)/block-module.wasm
.PHONY: wasm

# main application compilation
_main_path := $(_out_wasm)/main.wasm
$(_main_path): interface/entry-point.cpp $(self_objects) $(wasgen_objects)
	$(em_main) $< $(self_objects_em) $(wasgen_objects_em) -o $@

# setup the wasm for the server
server: $(_out_wasm)/glue-module.wasm $(_main_path)
.PHONY: server

# clean all produced output
clean:
	rm -rf server/wat
	rm -rf server/wasm
	rm -rf build
.PHONY: clean
