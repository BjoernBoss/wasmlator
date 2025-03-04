/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "../system.h"

static util::Logger logger{ u8"sys::userspace" };

bool sys::Userspace::fSetup(std::unique_ptr<sys::Userspace>&& system, std::unique_ptr<sys::Cpu>&& cpu, const sys::RunConfig& config, bool debug) {
	/* configure the new userspace object */
	pArgs = config.args;
	pEnvs = config.envs;
	pBinaryPath = config.binary;
	pCpu = cpu.get();

	/* log the configuration */
	logger.info(u8"  Cpu              : [", pCpu->name(), u8']');
	logger.info(u8"  Debug            : ", str::As{ U"S", debug });
	logger.info(u8"  Log Blocks       : ", str::As{ U"S", config.logBlocks });
	logger.info(u8"  Trace Blocks     : ", config.trace);
	logger.info(u8"  Translation Depth: ", config.translationDepth);
	logger.info(u8"  Binary           : ", pBinaryPath);
	logger.info(u8"  Arguments        : ", pArgs.size());
	for (size_t i = 0; i < pArgs.size(); ++i)
		logger.info(u8"               [", str::As{ U" 2", i }, u8"]: ", pArgs[i]);
	logger.info(u8"  Environments     : ", pEnvs.size());
	for (size_t i = 0; i < pEnvs.size(); ++i)
		logger.info(u8"               [", str::As{ U" 2", i }, u8"]: ", pEnvs[i]);

	/* check if the cpu can be set up */
	if (!pCpu->setupCpu(&pWriter)) {
		logger.error(u8"Failed to setup cpu [", pCpu->name(), u8"] for userspace execution");
		return false;
	}
	if (pCpu->multiThreaded())
		logger.warn(u8"Userspace currently does not support true multiple threads");

	/* validate the cpu-architecture */
	if (pCpu->architecture() == sys::ArchType::unknown) {
		logger.error(u8"Unsupported architecture [", fArchType(pCpu->architecture()), u8"] used by cpu");
		return false;
	}

	/* check if the debugger could be created */
	std::function<void(env::guest_t)> debugCheck;
	if (debug) {
		/* setup the debugger itself */
		if (!pDebugger.fSetup(this)) {
			logger.error(u8"Failed to attach debugger");
			return false;
		}
		logger.info(u8"Debugger attached successfully");

		/* setup the debug-check function */
		debugCheck = [debugger = &pDebugger](env::guest_t address) { debugger->fCheck(address); };
	}

	/* register the process and translator (translator first, as it will be used for core-creation) */
	if (!gen::SetInstance(std::move(cpu), config.translationDepth, config.trace, debugCheck))
		return false;
	if (env::SetInstance(std::move(system), detail::PageSize, pCpu->memoryCaches(), pCpu->contextSize(), pCpu->detectWriteExecute(), config.logBlocks))
		return true;
	gen::ClearInstance();
	return false;
}
std::u8string_view sys::Userspace::fArchType(sys::ArchType architecture) const {
	switch (architecture) {
	case sys::ArchType::riscv64:
		return u8"riscv64";
	case sys::ArchType::x86_64:
		return u8"x86_64";
	default:
		return u8"unknown";
	}
}
env::guest_t sys::Userspace::fPrepareStack() const {
	env::Memory& mem = env::Instance()->memory();
	size_t wordWidth = ((pLoaded.bitWidth == 64) ? 8 : 4);

	/* initialize the content for the stack */
	std::vector<uint8_t> content;
	auto writeBytes = [&](const void* data, size_t size) -> size_t {
		const uint8_t* d = static_cast<const uint8_t*>(data);
		size_t offset = content.size();
		content.insert(content.end(), d, d + size);
		return offset;
		};
	auto writeWord = [&](uint64_t value) -> size_t {
		const uint8_t* d = reinterpret_cast<const uint8_t*>(&value);
		size_t offset = content.size();
		content.insert(content.end(), d, d + wordWidth);
		return offset;
		};
	auto changeWord = [&](size_t offset, uint64_t value) {
		value += *reinterpret_cast<const uint64_t*>(content.data() + offset);
		const uint8_t* d = reinterpret_cast<const uint8_t*>(&value);
		std::copy(d, d + wordWidth, content.begin() + offset);
		};

	/* write the argument-count out */
	writeWord(pArgs.size() + 1);

	/* write the binary path out */
	uint64_t offsetOfPath = writeWord(0x00);

	/* write the argument pointers out and the trailing null-entry */
	uint64_t startOfArgs = content.size();
	for (size_t i = 0; i < pArgs.size(); ++i)
		writeWord(0x00);
	writeWord(0);

	/* write the environment pointers out and the trailing null-entry */
	uint64_t startOfEnvs = content.size();
	for (size_t i = 0; i < pEnvs.size(); ++i)
		writeWord(0x00);
	writeWord(0);

	/* write the auxiliary vector entries out */
	writeWord(uint64_t(linux::AuxiliaryType::phAddress));
	writeWord(pLoaded.aux.phAddress);
	writeWord(uint64_t(linux::AuxiliaryType::phEntrySize));
	writeWord(pLoaded.aux.phEntrySize);
	writeWord(uint64_t(linux::AuxiliaryType::phCount));
	writeWord(pLoaded.aux.phCount);
	writeWord(uint64_t(linux::AuxiliaryType::pageSize));
	writeWord(pLoaded.aux.pageSize);
	writeWord(uint64_t(linux::AuxiliaryType::baseInterpreter));
	writeWord(pLoaded.aux.base);
	writeWord(uint64_t(linux::AuxiliaryType::flags));
	writeWord(0);
	writeWord(uint64_t(linux::AuxiliaryType::entry));
	writeWord(pLoaded.aux.entry);
	writeWord(uint64_t(linux::AuxiliaryType::uid));
	writeWord(pSyscall.process().uid);
	writeWord(uint64_t(linux::AuxiliaryType::euid));
	writeWord(pSyscall.process().euid);
	writeWord(uint64_t(linux::AuxiliaryType::gid));
	writeWord(pSyscall.process().gid);
	writeWord(uint64_t(linux::AuxiliaryType::egid));
	writeWord(pSyscall.process().egid);
	writeWord(uint64_t(linux::AuxiliaryType::random));
	size_t offsetOfRandom = writeWord(0x00);
	writeWord(uint64_t(linux::AuxiliaryType::secure));
	writeWord(0);
	writeWord(uint64_t(linux::AuxiliaryType::executableFilename));
	size_t offsetOfExecutable = writeWord(0x00);

	/* write the auxiliary vector null-entry out */
	writeWord(0);

	/* write the padding out to 16 bytes */
	content.insert(content.end(), 16 - (content.size() % 16), 0);

	/* write the random data out and write its offset back to its location */
	changeWord(offsetOfRandom, content.size());
	for (size_t i = 0; i < 4; ++i) {
		uint32_t tmp = host::GetRandom();
		writeBytes(&tmp, 4);
	}

	/* write the argument binary out and the offset back to its location */
	changeWord(offsetOfPath, writeBytes(pBinaryPath.data(), pBinaryPath.size() + 1));

	/* write the arguments out and the offsets back to their locations */
	for (size_t i = 0; i < pArgs.size(); ++i)
		changeWord(startOfArgs + i * wordWidth, writeBytes(pArgs[i].data(), pArgs[i].size() + 1));

	/* write the environments out and the offsets back to their locations */
	for (size_t i = 0; i < pEnvs.size(); ++i)
		changeWord(startOfEnvs + i * wordWidth, writeBytes(pEnvs[i].data(), pEnvs[i].size() + 1));

	/* write the name of the actual executable out and the offset back to its location */
	changeWord(offsetOfExecutable, writeBytes(pBinaryActual.data(), pBinaryActual.size() + 1));

	/* append the remaining padding to fully align the stack */
	content.insert(content.end(), detail::StartOfStackAlignment - (content.size() % detail::StartOfStackAlignment), 0);

	/* check if the content fits into the stack-buffer */
	if (content.size() > detail::StackSize) {
		logger.error(u8"Cannot fit [", content.size(), u8"] bytes onto the initial stack of size [", detail::StackSize, u8']');
		return 0;
	}

	/* allocate the new stack */
	env::guest_t stackBase = mem.alloc(detail::StackSize, env::Usage::ReadWrite);
	if (stackBase == 0) {
		logger.error(u8"Failed to allocate initial stack");
		return 0;
	}
	logger.debug(u8"Stack allocated to [", str::As{ U"#018x", stackBase }, u8"] with size [", str::As{ U"#010x", detail::StackSize }, u8']');

	/* compute the final base-address of the stack and patch the addresses */
	env::guest_t stack = stackBase + detail::StackSize - content.size();
	changeWord(offsetOfRandom, stack);
	changeWord(offsetOfPath, stack);
	for (size_t i = 0; i < pArgs.size(); ++i)
		changeWord(startOfArgs + i * wordWidth, stack);
	for (size_t i = 0; i < pEnvs.size(); ++i)
		changeWord(startOfEnvs + i * wordWidth, stack);
	changeWord(offsetOfExecutable, stack);

	/* write the prepared content to the acutal stack */
	mem.mwrite(stack, content.data(), uint32_t(content.size()), env::Usage::Write);

	/* log the stack-state */
	for (size_t i = 0; i < content.size(); i += 8)
		logger.trace(u8"Stack [", str::As{ U"#018x", stack + i }, u8"] : ", str::As{ U"#018x", *(uint64_t*)(content.data() + i) });
	return stack;
}
void sys::Userspace::fStartLoad(const std::u8string& path) {
	/* setup the actual path to use */
	std::u8string actual = util::CanonicalPath(str::u8::Build(detail::ResolveLocations[pResolveIndex], path));

	/* load the binary */
	env::Instance()->filesystem().readStats(actual, [this, path, actual](const env::FileStats* stats) {
		/* check if the file could be resolved */
		if (stats == 0) {
			/* check if another path should be resolved (only if the name is raw) */
			bool fileOnly = (path.find(u8'/') == std::u8string::npos && path.find(u8'\\') == std::u8string::npos);
			if (pResolveIndex + 1 < std::size(detail::ResolveLocations) && fileOnly) {
				logger.error(str::u8::Build(u8"Path [", actual, u8"] does not exist"));
				++pResolveIndex;
				fStartLoad(path);
				return;
			}

			/* fail the loading but ensure that the error is displayed to the user, no matter if loggin is enabled or not */
			std::u8string msg = str::u8::Build(u8"Path [", path, u8"] does not exist");
			if (!logger.error(msg))
				host::PrintOutLn(msg);
			env::Instance()->shutdown();
			return;
		}

		/* reset the resolve-index to allow future resolves to restart again */
		pResolveIndex = 0;

		/* check the file-type */
		if (stats->type != env::FileType::file) {
			/* this error should be displayed to the user, no matter if logging is enabled or not */
			std::u8string msg = str::u8::Build(u8"Path [", actual, u8"] is not a file");
			if (!logger.error(msg))
				host::PrintOutLn(msg);
			env::Instance()->shutdown();
			return;
		}

		/* allocate the buffer for the file-content and read it into memory */
		uint8_t* buffer = new uint8_t[stats->size];
		env::Instance()->filesystem().readFile(stats->id, 0, buffer, stats->size, [this, size = stats->size, actual, buffer](std::optional<uint64_t> read) {
			std::unique_ptr<uint8_t[]> _cleanup{ buffer };

			/* check if the size still matches */
			if (size != read) {
				/* this error should be displayed to the user, no matter if logging is enabled or not */
				std::u8string_view msg = (read.has_value() ? u8"Unable to read entire file" : u8"Error while reading file");
				if (!logger.error(msg))
					host::PrintOutLn(msg);
				env::Instance()->shutdown();
				return;
			}

			/* perform the actual loading of the file */
			if (!fBinaryLoaded(actual, buffer, size))
				env::Instance()->shutdown();
			});
		});
}
bool sys::Userspace::fBinaryLoaded(const std::u8string& actual, const uint8_t* data, size_t size) {
	/* log the successful load and write the path back */
	if (pLoaded.interpreter.empty()) {
		pBinaryActual = actual;
		logger.debug(u8"Binary loading completed with [", pBinaryPath, u8"] being at at [", pBinaryActual, u8']');
	}
	else
		logger.debug(u8"Interpreter loading completed with [", pLoaded.interpreter, u8"] being at at [", actual, u8']');

	/* load the elf-image and configure the startup-address */
	try {
		/* check if just the interpreter needs to be loaded (no need to perform architecture checks again - as it will remain unchanged) */
		if (!pLoaded.interpreter.empty()) {
			sys::LoadElfInterpreter(pLoaded, data, size);
			logger.debug(u8"Entry of interpreter: ", str::As{ U"#018x", pLoaded.start });
			return fLoadCompleted();
		}

		/* load the elf */
		pLoaded = sys::LoadElf(data, size);
		logger.debug(u8"Entry of program   : ", str::As{ U"#018x", pLoaded.start });
		logger.debug(u8"Start of heap      : ", str::As{ U"#018x", pLoaded.endOfData });
	}
	catch (const elf::Exception& e) {
		/* this error should be displayed to the user, no matter if logging is enabled or not */
		std::u8string msg = str::u8::Build(u8"Error while loading elf: ", e.what());
		if (!logger.error(msg))
			host::PrintOutLn(msg);
		return false;
	}

	/* validate the word-width (necessary, as env::Memory's allocate function currently
	*	allocates into the unreachable portion of the 32 bit address space) */
	if (pLoaded.bitWidth != 64) {
		/* this error should be displayed to the user, no matter if logging is enabled or not */
		std::u8string_view msg = u8"Only 64 bit elf binaries are supported";
		if (!logger.error(msg))
			host::PrintOutLn(msg);
		return false;
	}

	/* validate the machine type and match it */
	sys::ArchType arch = sys::ArchType::unknown;
	switch (pLoaded.machine) {
	case elf::MachineType::riscv:
		arch = sys::ArchType::riscv64;
		break;
	case elf::MachineType::x86_64:
		arch = sys::ArchType::x86_64;
		break;
	default:
		arch = sys::ArchType::unknown;
		break;
	}

	/* check if the type matches */
	if (pCpu->architecture() != arch) {
		/* this error should be displayed to the user, no matter if logging is enabled or not */
		std::u8string msg = str::u8::Build(u8"Cpu for architecture [", fArchType(pCpu->architecture()), u8"] cannot execute elf of type [", fArchType(arch), u8']');
		if (!logger.error(msg))
			host::PrintOutLn(msg);
		return false;
	}

	/* check if an interpreter needs to be loaded or if the elf has been loaded successfully */
	if (pLoaded.interpreter.empty())
		return fLoadCompleted();
	logger.info(u8"Binary requires interpreter [", pLoaded.interpreter, u8']');
	fStartLoad(pLoaded.interpreter);
	return true;
}
bool sys::Userspace::fLoadCompleted() {
	/* initialize the starting address */
	pAddress = pLoaded.start;

	/* initialize the stack based on the system-v ABI stack specification (architecture independent) */
	env::guest_t spAddress = fPrepareStack();
	if (spAddress == 0)
		return false;
	logger.debug(u8"Stack loaded to: ", str::As{ U"#018x", spAddress });

	/* initialize the syscall environment */
	if (!pSyscall.setup(this, pLoaded.endOfData, pBinaryActual, fArchType(pCpu->architecture()))) {
		logger.error(u8"Failed to setup the userspace syscalls");
		return false;
	}

	/* initialize the context */
	if (!pCpu->setupContext(pAddress, spAddress))
		return false;

	/* log the system as fully loaded */
	logger.log(u8"Userspace environment fully initialized");

	/* startup the translation and execution of the blocks */
	env::Instance()->startNewBlock();
	return true;
}

void sys::Userspace::fCheckContinue() const {
	/* check if the memory detected an invalidation */
	env::Instance()->memory().checkXInvalidated(pAddress);
}
void sys::Userspace::fExecute() {
	/* start execution of the next address and catch/handle any incoming exceptions */
	try {
		/* check if the execution can simply continue and execute the next address */
		fCheckContinue();
		env::Instance()->mapping().execute(pAddress);
	}
	catch (const env::Terminated& e) {
		pAddress = e.address;
		logger.log(u8"Execution terminated at [", str::As{ U"#018x", e.address }, u8"] with [", e.code, u8']');

		/* shutdown the system */
		logger.log(u8"Shutting userspace environment down");
		env::Instance()->shutdown();
	}
	catch (const env::MemoryFault& e) {
		pAddress = e.address;
		logger.fmtFatal(u8"MemoryFault detected at: [{:#018x}] while accessing [{:#018x}] as [{}] while page is mapped as [{}]",
			e.address, e.accessed, env::Usage::Print{ e.usedUsage }, env::Usage::Print{ e.actualUsage });
	}
	catch (const env::Decoding& e) {
		pAddress = e.address;
		logger.fatal(u8"Decoding caught: [", str::As{ U"#018x", e.address }, u8"] - [", (e.memoryFault ? u8"Memory-Fault" : u8"Decoding-Fault"), u8']');
	}
	catch (const env::Translate& e) {
		pAddress = e.address;
		logger.debug(u8"Translate caught: [", str::As{ U"#018x", e.address }, u8']');
		env::Instance()->startNewBlock();
	}
	catch (const env::ExecuteDirty& e) {
		pAddress = e.address;
		logger.debug(u8"Flushing instruction cache");
		env::Instance()->mapping().flush();
		env::Instance()->startNewBlock();
	}
	catch (const detail::CpuException& e) {
		pAddress = e.address;
		logger.fatal(u8"CPU Exception caught: [", str::As{ U"#018x", e.address }, u8"] - [", pCpu->getExceptionText(e.id), u8']');
	}
	catch (const detail::UnknownSyscall& e) {
		pAddress = e.address;
		logger.fatal(u8"Unknown syscall caught: [", str::As{ U"#018x", e.address }, u8"] - [Index: ", e.index, u8']');
	}
	catch (const detail::AwaitingSyscall&) {
		logger.trace(u8"Awaiting syscall result at [", str::As{ U"#018x", pAddress }, u8']');
	}
	catch (const detail::DebuggerHalt&) {
		logger.trace(u8"Debugger halted at [", str::As{ U"#018x", pAddress }, u8']');
	}

	/* will onlybe reached through: New block being generated, DebuggerHalt, AwaitingSyscall */
}

bool sys::Userspace::Create(std::unique_ptr<sys::Cpu>&& cpu, const sys::RunConfig& config) {
	std::unique_ptr<sys::Userspace> system{ new sys::Userspace() };

	/* try to setup the system */
	sys::Userspace* _this = system.get();
	return _this->fSetup(std::move(system), std::move(cpu), config, false);
}
sys::Debugger* sys::Userspace::Debug(std::unique_ptr<sys::Cpu>&& cpu, const sys::RunConfig& config) {
	std::unique_ptr<sys::Userspace> system{ new sys::Userspace() };

	/* try to setup the system */
	sys::Userspace* _this = system.get();
	if (!_this->fSetup(std::move(system), std::move(cpu), config, true))
		return 0;
	return &_this->pDebugger;
}

bool sys::Userspace::setupCore(wasm::Module& mod) {
	/* setup the actual core */
	gen::Core core{ mod };

	/* setup the writer object (before setting up cpu, as it might use the writer) */
	if (!pWriter.fSetup(&pSyscall)) {
		logger.error(u8"Failed to setup the userspace-writer");
		return false;
	}

	/* perform the cpu-configuration of the core */
	if (!pCpu->setupCore(mod))
		return false;

	/* finalize the actual core */
	return core.close();
}
void sys::Userspace::coreLoaded() {
	fStartLoad(pBinaryPath);
}
std::vector<env::BlockExport> sys::Userspace::setupBlock(wasm::Module& mod) {
	/* setup the translator */
	gen::Block translator{ mod };

	/* translate the next requested address */
	translator.run(pAddress);

	/* finalize the translation */
	return translator.close();
}
void sys::Userspace::blockLoaded() {
	fExecute();
}
void sys::Userspace::shutdown() {
	/* clearing the system-reference will also release this object */
	gen::ClearInstance();
	env::ClearInstance();
}

const sys::Cpu* sys::Userspace::cpu() const {
	return pCpu;
}
sys::Cpu* sys::Userspace::cpu() {
	return pCpu;
}
env::guest_t sys::Userspace::getPC() const {
	return pAddress;
}
void sys::Userspace::setPC(env::guest_t address) {
	pAddress = address;
}
void sys::Userspace::execute() {
	fExecute();
}
void sys::Userspace::checkContinue() {
	fCheckContinue();
}
