# Dynamic Binary Translator to WebAssembly written in C++
![C++](https://img.shields.io/badge/language-c%2B%2B20-blue?style=flat-square)
[![License](https://img.shields.io/badge/license-BSD--3--Clause-brightgreen?style=flat-square)](LICENSE.txt)

This is a dynamic binary translator primarily written in `C++20`, which translated from `RISCV-64` to `WebAssembly`. It is capable of loading userspace linux ELF binaries and emulates all necessary syscalls to allow for proper execution to occur.

## Cloning the Project
To clone the project, simply clone the repository and initialize the submodules. Note: because all of the submodules use a common submodule [`ustring`](https://github.com/BjoernBoss/ustring), it is adviced to not clone recursively, as this will clone the repository three times into the submodules.

	$ git clone https://github.com/BjoernBoss/wasmlator.git
	$ cd wasmlator
	$ git submodule init
	$ git submodule update

## Building the Project
To build the project, the following tools are required. They are assumed to be reachable from the command line as written (i.e. `python` instead of `python3` or similar names).

- `make`: The primary build is implemented via a Makefile
- `tsc`: Core components of the applications interface with the environment are written in TypeScript.
- `python`: The Makefile uses python internally and the provided web server is written in python as well.
- `em++`: Emscripten is used to compile the main application to WebAssembly.
- `clang++`: Clang is used to compile intermediate helping tools to generate precompiled WebAssembly modules.

To build the entire library, simply run:

	$ make all

To only build the Node.js, run:

	$ make node

And for only the web browser implementation, run:

	$ make web

These build commands will create a `build` directory, and generate all necessary components to it. To remove any build artifacts, use:

	$ make clean

Note: The Makefile contains an embedded python script, which recursively crawls the repository, and creates an intermediate Makefile to `build/make/generated.make`. This Makefile contains all encountered `cpp` files, and adds all included files (and their includes) as prerequisites. Should new `cpp` files be added, or others be removed, simply delete the generated Makefile or perform a clean.

## Generic Information about Running the Project
After the build has been completed, all necessary files will have been generated to `build/exec`. This includes the `server.py` for the web browser interactions, and `main.js` for loading using Node.js.

The `build/exec` directory will also contain a file `fs.root`. It must contain the path to directory to be used as root for the file system as seen by the guest.

`Important:` As of now, the entire file system acts as a snapshot for the guest. The files will be fetched from the location provided by `fs.root`, but no changes will be written out. All added, modified, or deleted files will be held in memory, and these changes will be lost when the execution is terminated.

Both the Node.js and web server implementation expect to be executed from the root of the repository.

`Note:` As the terminals provided by Node.js and the web server are not actual shells, they do not provide any path resolving hints. The guest is executed from the directory it lies in (within the virtual file system). Any path arguments to the guest must therefore either be given as absolute paths, or relative to the binary.

## Running the Node.js Implementation
To run the Node.js implementation, simply execute the following command from the root of the repository:

	$ node build/exec/main.js

This will start an interactive menu to load and execute binaries in the virtual file system. Alternatively, the binary can be given as an immediate argument:

	$ node build/exec/main.js /bin/ls /

## Running the Web Server Implementation
This is not an ordinary static web server. Instead, it also provides more complex endpoints, which allow the web implementation to access and query the virtual file system.

To start the web server itself, run:

	$ python build/exec/server.py

This will provide two HTML pages: terminal and interact. All changes within the virtual file system are preserved until the given website is closed.

### Web Server Terminal
This page will be available at [`http://localhost:9090/terminal.html`](http://localhost:9090/terminal.html). It acts like the Node.js interactive terminal, and allows binaries to be executed.

### Web Server Interact
This page will be available at [`http://localhost:9090/interact.html`](http://localhost:9090/interact.html). It allows the logging output to be viewed, as well as the application to be debugged and inspected.

Due to the vast amount of logging, the page also includes toggle buttons at the top, which allow configuring the corresponding logging class. 

- Light Gray: The logging is enabled and will be displayed on the screen
- Dark Gray: The logging is hidden, can be enabled by toggling back to light gray, but the messages are not discarded.
- Dark Red: The logging is hidden and all newly received logs of the class are discarded. Note: already logged but hidden messages are not removed, they can still be enabled by toggling back to light gray.
- Clear Button: Delete all logged information - hidden or not.

Use `help` to view all of the supported commands.
