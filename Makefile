# python script, which reads all of the cpp-files, recursively processes the
# includes, and generates a make-file to create all of the corresponding object-files
py_gen_make_file := "import os; import re; import sys\nv = {}\ndef c(p):\n if p in v: return\
\n v[p] = set(t for t in (os.path.relpath(os.path.join('./repos', i)).replace('\\u005c', '/') for i in re.findall('\\u0023include <(.*)>', open(p, 'r').read())) if os.path.isfile(t))\
\n v[p].update(t for t in (os.path.relpath(os.path.join(os.path.dirname(p), i)).replace('\\u005c', '/') for i in re.findall('\\u0023include \\u0022(.*)\\u0022', open(p, 'r').read())) if os.path.isfile(t))\
\n for t in v[p]:\n  c(t)\nfor (dir, _, files) in os.walk('.'):\n for f in files:\n  if f.endswith('.cpp'):\n   c(os.path.relpath(os.path.join(dir, f)).replace('\\u005c', '/'))\nwhile True:\
\n dirty = False\n for p in v:\n  o = v[p].copy()\n  for t in v[p]:\n   o.update(v[t])\n  if len(o) != len(v[p]):\n   dirty = True\n  v[p] = o\n if not dirty: break\
\nf, l, o = open(sys.argv[1], 'w'), [], []\nfor p in v:\n if not p.endswith('.cpp'): continue\n if p.find('entry/') != -1:\n  o.append(p)\n  continue\n n = os.path.basename(p)[:-4]\
\n f.write(f'\\u0024(cc_path)/{n}.o: {p} ' + ' '.join(t for t in v[p]) + ' | \\u0024(cc_path)\\u000a')\n f.write(f'\\u0009\\u0024(cc) -c \\u0024< -o \\u0024@\\u000a')\
\n f.write(f'\\u0024(em_path)/{n}.o: {p} ' + ' '.join(t for t in v[p]) + ' | \\u0024(em_path)\\u000a')\n f.write(f'\\u0009\\u0024(em) -c \\u0024< -o \\u0024@\\u000a')\n l.append(n)\
\nf.write('\\u000a')\nf.write(f'obj_list_cc := ' + ' '.join(f'\\u0024(cc_path)/{n}.o' for n in l) + '\\u000a')\nf.write(f'obj_list_em := ' + ' '.join(f'\\u0024(em_path)/{n}.o' for n in l) + '\\u000a')\
\nf.write('\\u000a')\nf.writelines(os.path.basename(p)[:-4].replace('-', '_') + f'_prerequisites := {p} ' + ' '.join(t for t in v[p]) + '\\u000a' for p in o)"

# relevant paths referenced throughout this script
build_path := build
fs_path := run/fs
gen_path := run/generated
ts_path := run/backend
bin_path := $(build_path)/make
cc_path := $(build_path)/make/cc
em_path := $(build_path)/make/em

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
	@echo ""
	@echo "Note:"
	@echo "   This makefile will create and include a generated makefile, which is generated"
	@echo "   through inlined python. This makefile will automatically be generated for the"
	@echo "   (server/wat/wasm) targets. Should the fundamental structure of the repository"
	@echo "   change, simply perform a full clean and full creation to update the script."
.PHONY: help

# clean all produced output
clean:
	rm -rf $(wat_path)
	rm -rf $(wasm_path)
	rm -rf $(build_path)
.PHONY: clean

# path-creating targets
$(cc_path):
	@ mkdir -p $(cc_path)
$(em_path):
	@ mkdir -p $(em_path)
$(gen_path):
	@ mkdir -p $(gen_path)
$(bin_path):
	@ mkdir -p $(bin_path)
$(build_path):
	@ mkdir -p $(build_path)
$(fs_path):
	@ mkdir -p $(fs_path)

# default emscripten compiler with all relevant flags
em := em++ -std=c++20 -I./repos -O1 -fwasm-exceptions
em_main := $(em) --no-entry -sERROR_ON_UNDEFINED_SYMBOLS=0 -sWARN_ON_UNDEFINED_SYMBOLS=0 -sWASM_BIGINT -sALLOW_MEMORY_GROWTH -sSTANDALONE_WASM\
 -sEXPORTED_FUNCTIONS=_main_user_command,_main_task_completed,_main_allocate,_main_terminate,_main_code_exception,_main_resolve,_main_check_lookup,_main_fast_lookup,_main_check_invalidated,_main_invoke_void,_main_invoke_param

# default clang-compiler with all relevant flags
cc := clang++ -std=c++20 -O3 -I./repos

# execute the python-script to generate the make-file, if it does not exist yet
generated_path := $(build_path)/generated.make
$(generated_path): | $(build_path)
	@echo Generating... $@
	@ py -c "$$(echo $(py_gen_make_file) | sed 's/\\\\n/\\n/g')" $@

# include the generated build-script, which builds all separate objects
# (references cc/cc_path/em/em_path and produces all *_prerequisites, as well as obj_list_cc and obj_list_em)
ifneq ($(subst clean,,$(subst help,,$(MAKECMDGOALS))),)
include $(generated_path)
endif

# glue-generator compilation
glue_gen_path := $(bin_path)/glue.exe
$(glue_gen_path): $(make_glue_prerequisites) $(null_interface_prerequisites) $(obj_list_cc)
	@echo Compiling... $@
	@ $(cc) entry/make-glue.cpp entry/null-interface.cpp $(obj_list_cc) -o $@

# main wasm compilation
main_path := $(gen_path)/main.wasm
$(main_path): $(obj_list_em) | $(gen_path)
	@echo Compiling... $@
	@ $(em_main) $(obj_list_em) -o $@

# glue wasm generation
glue_path := $(gen_path)/glue.wasm
$(glue_path): $(glue_gen_path) | $(gen_path)
	@echo Generating... $@
	@ $(glue_gen_path) $@

# javascript generation
wasmlator_path := $(gen_path)/wasmlator.js
$(wasmlator_path): $(ts_path)/tsconfig.json $(ts_path)/*.ts
	@echo Generating... $@
	@ tsc -p $<

# setup the wasm for the server
server: $(main_path) $(glue_path) $(wasmlator_path) | $(fs_path)
.PHONY: server
