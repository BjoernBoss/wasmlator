# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 Bjoern Boss Henrichsen
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
run_src := run
out_path := $(build_path)/exec

# help-menu
help:
	@ echo "Usage: make [target]"
	@ echo ""
	@ echo "TARGETS:"
	@ echo "   help    print this menu; default"
	@ echo "   clean   remove all build outputs"
	@ echo "   web     generate all files to [$(out_path)] necessary to run the server [$(out_path)/server.py]"
	@ echo "   node    generate all files to [$(out_path)] necessary to run the node runner [$(out_path)/main.js]"
	@ echo "   all     generate all targets to [$(out_path)]"
	@ echo ""
	@ echo "Note:"
	@ echo "   This makefile will create and include a generated makefile, which is generated"
	@ echo "   through inlined python. This makefile will automatically be generated for the"
	@ echo "   server target. Should the fundamental structure of the repository change,"
	@ echo "   simply perform a full clean and full creation to update the script."
.PHONY: help

# clean all produced output
clean:
	rm -r $(build_path)
.PHONY: clean

# default emscripten compiler with all relevant flags
em := em++ -std=c++20 -I./repos -O3 -fwasm-exceptions
em_main := $(em) --no-entry -sERROR_ON_UNDEFINED_SYMBOLS=0 -sWARN_ON_UNDEFINED_SYMBOLS=0 -sWASM_BIGINT -sALLOW_MEMORY_GROWTH -sSTANDALONE_WASM\
 -sEXPORTED_FUNCTIONS=_main_handle,_main_execute,_main_cleanup,_main_task_completed,_main_allocate,_main_terminate,_main_code_exception,_main_resolve,_main_check_lookup,_main_fast_lookup,_main_check_invalidated,_main_invoke_void,_main_invoke_param

# default clang-compiler with all relevant flags
cc := clang++ -std=c++20 -O3 -I./repos

# execute the python-script to generate the make-file, if it does not exist yet
make_path := $(build_path)/make
generated_path := $(make_path)/generated.make
$(generated_path):
	@ echo Generating... $@
	@ mkdir -p $(make_path)
	@ py -c "$$(echo $(py_gen_make_file) | sed 's/\\\\n/\\n/g')" $@

# paths used for the output of the generated make file builds
cc_path := $(make_path)/cc
em_path := $(make_path)/em
$(cc_path):
	@ mkdir -p $@
$(em_path):
	@ mkdir -p $@

# include the generated build-script, which builds all separate objects
# (references cc/cc_path/em/em_path and produces all *_prerequisites, as well as obj_list_cc and obj_list_em)
ifneq ($(subst clean,,$(subst help,,$(MAKECMDGOALS))),)
include $(generated_path)
endif

# glue-generator compilation
glue_gen_path := $(make_path)/glue.exe
$(glue_gen_path): $(make_glue_prerequisites) $(null_interface_prerequisites) $(obj_list_cc)
	@ echo Compiling... $@
	@ mkdir -p $(make_path)
	@ $(cc) entry/make-glue.cpp entry/null-interface.cpp $(obj_list_cc) -o $@

# main wasm compilation
wasm_path := $(out_path)/wasm
main_path := $(wasm_path)/main.wasm
$(main_path): $(obj_list_em)
	@ echo Compiling... $@
	@ mkdir -p $(dir $@)
	@ $(em_main) $(obj_list_em) -o $@

# glue wasm generation
glue_path := $(wasm_path)/glue.wasm
$(glue_path): $(glue_gen_path)
	@ echo Generating... $@
	@ mkdir -p $(dir $@)
	@ $(glue_gen_path) $@

# target to generate all wasm
build_wasm: $(main_path) $(glue_path)
.PHONY: build_wasm

# creation of the filesystem root indication file
fs_indicator_path := $(out_path)/fs.root
$(fs_indicator_path):
	@ echo Generating... $@
	@ mkdir -p $(out_path)
	@ echo "../../fs" > $@

# common javascript generation
common_in_path := $(run_src)/common
common_out_path := $(out_path)/common
common_file := $(common_out_path)/wasmlator.js
$(common_file): $(common_in_path)/tsconfig.json $(wildcard $(common_in_path)/*.ts)
	@ echo Generating... $@
	@ mkdir -p $(common_out_path)
	@ tsc -p $< --outDir $(common_out_path)

# target to build the common typescript components
build_common: $(common_file)
.PHONY: build_common

# output copy template (used to copy files directly to exec directory)
define copy_template
$(1): $(2)
	@ echo Copying... $(1)
	@ mkdir -p $(dir $(1))
	@ cp $(2) $(1)
endef

# output copy template with placeholder replacement (used to copy files directly to exec directory but replacing necessary placeholders)
define copy_template_placeholders
$(1): $(2)
	@ echo Copying... $(1)
	@ mkdir -p $(dir $(1))
	@ sed \
	-e "s|{{root-indicator-path}}|$(fs_indicator_path)|g" \
	-e "s|{{exec-path}}|$(out_path)|g" \
	-e "s|{{wasm-path}}|$(wasm_path)|g" \
	-e "s|{{common-path}}|$(common_out_path)|g" \
	-e "s|{{common-exec-rel-path}}|$(subst $(out_path)/,,$(common_out_path))|g" \
	-e "s|{{self-path}}|$(3)|g" \
	-e "s|{{self-exec-rel-path}}|$(subst $(out_path)/,,$(3))|g" \
	$(2) > $(1)
endef

# nodejs copy to output
node_in_path := $(run_src)/nodejs
node_out_path := $(out_path)/node
node_parts := $(subst $(node_in_path)/components/,,$(wildcard $(node_in_path)/components/*.*) $(wildcard $(node_in_path)/components/*/*.*))
$(foreach file, $(node_parts), $(eval $(call copy_template,$(node_out_path)/$(file),$(node_in_path)/components/$(file))))
node_files := $(subst $(node_in_path)/,,$(wildcard $(node_in_path)/*.*))
$(foreach file, $(node_files), $(eval $(call copy_template_placeholders,$(out_path)/$(file),$(node_in_path)/$(file),$(node_out_path))))

# target to copy all node components over
node_copy: $(foreach file,$(node_files),$(out_path)/$(file)) $(foreach file,$(node_parts),$(node_out_path)/$(file))
.PHONY: node_copy

# target to build everything required for the webserver
web: web_copy build_common build_wasm $(fs_indicator_path)
.PHONY: web

# web copy to output
web_in_path := $(run_src)/web
web_out_path := $(out_path)/web
web_parts := $(subst $(web_in_path)/static/,,$(wildcard $(web_in_path)/static/*.*) $(wildcard $(web_in_path)/static/*/*.*))
$(foreach file, $(web_parts), $(eval $(call copy_template,$(web_out_path)/$(file),$(web_in_path)/static/$(file))))
web_files := $(subst $(web_in_path)/,,$(wildcard $(web_in_path)/*.*))
$(foreach file, $(web_files), $(eval $(call copy_template_placeholders,$(out_path)/$(file),$(web_in_path)/$(file),$(web_out_path))))

# target to copy all web components over
web_copy: $(foreach file,$(web_files),$(out_path)/$(file)) $(foreach file, $(web_parts), $(web_out_path)/$(file))
.PHONY: web_copy

# target to build everything required for the node.js runner
node: node_copy build_common build_wasm $(fs_indicator_path)
.PHONY: node

# target to build all output types
all: web node
.PHONY: all
