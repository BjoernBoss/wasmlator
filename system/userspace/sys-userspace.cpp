#include "../system.h"

static util::Logger logger{ u8"sys::userspace" };

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

	/* count the size of the blob of data at the end (16 bytes for random data) */
	size_t blobSize = 16 + 2 * (pBinary.size() + 1);
	for (size_t i = 0; i < pArgs.size(); ++i)
		blobSize += pArgs[i].size() + 1;
	for (size_t i = 0; i < pEnvs.size(); ++i)
		blobSize += pEnvs[i].size() + 1;

	/* compute the size of the structured stack-data
	*	Args: one pointer for each argument plus count plus null-entry plus self-path
	*	Envs: one pointer for each environment plus null-entry
	*	Auxiliary: (AT_PHDR/AT_PHNUM/AT_PHENT/AT_ENTRY/AT_RANDOM/AT_PAGESZ/AT_BASE/AT_EXECFN) one null-entry */
	size_t structSize = 0;
	structSize += wordWidth * (pArgs.size() + 3);
	structSize += wordWidth * (pEnvs.size() + 1);
	structSize += wordWidth * 16 + wordWidth;

	/* validate the stack dimensions (aligned to the corresponding boundaries) */
	size_t totalSize = (blobSize + structSize + Userspace::StartOfStackAlignment - 1) & ~(Userspace::StartOfStackAlignment - 1);
	size_t padding = totalSize - (blobSize + structSize);
	if (totalSize > Userspace::StackSize) {
		logger.error(u8"Cannot fit [", totalSize, u8"] bytes onto the initial stack of size [", Userspace::StackSize, u8']');
		return 0;
	}

	/* allocate the new stack */
	env::guest_t stackBase = mem.alloc(Userspace::StackSize, env::Usage::ReadWrite);
	if (stackBase == 0) {
		logger.error(u8"Failed to allocate initial stack");
		return 0;
	}
	logger.debug(u8"Stack allocated to [", str::As{ U"#018x", stackBase }, u8"] with size [", str::As{ U"#010x", Userspace::StackSize }, u8']');

	/* initialize the content for the stack */
	std::vector<uint8_t> content;
	auto writeBytes = [&](const void* data, size_t size) {
		const uint8_t* d = static_cast<const uint8_t*>(data);
		content.insert(content.end(), d, d + size);
		};
	auto writeWord = [&](uint64_t value) {
		const uint8_t* d = reinterpret_cast<const uint8_t*>(&value);
		content.insert(content.end(), d, d + wordWidth);
		};
	env::guest_t blobPtr = stackBase + Userspace::StackSize - blobSize;

	/* write the argument-count out */
	writeWord(pArgs.size() + 1);

	/* write the binary path out */
	writeWord(blobPtr);
	blobPtr += pBinary.size() + 1;

	/* write the argument pointers out and the trailing null-entry */
	for (size_t i = 0; i < pArgs.size(); ++i) {
		writeWord(blobPtr);
		blobPtr += pArgs[i].size() + 1;
	}
	writeWord(0);

	/* write the environment pointers out and the trailing null-entry */
	for (size_t i = 0; i < pEnvs.size(); ++i) {
		writeWord(blobPtr);
		blobPtr += pEnvs[i].size() + 1;
	}
	writeWord(0);

	/* write the auxiliary vector entries out */
	writeWord(uint64_t(linux::AuxiliaryType::phAddress));
	writeWord(pLoaded.aux.phAddress);
	writeWord(uint64_t(linux::AuxiliaryType::phCount));
	writeWord(pLoaded.aux.phCount);
	writeWord(uint64_t(linux::AuxiliaryType::phEntrySize));
	writeWord(pLoaded.aux.phEntrySize);
	writeWord(uint64_t(linux::AuxiliaryType::entry));
	writeWord(pLoaded.aux.entry);
	writeWord(uint64_t(linux::AuxiliaryType::baseInterpreter));
	writeWord(pLoaded.aux.base);
	writeWord(uint64_t(linux::AuxiliaryType::pageSize));
	writeWord(pLoaded.aux.pageSize);
	writeWord(uint64_t(linux::AuxiliaryType::executableFilename));
	writeWord(blobPtr);
	blobPtr += pBinary.size() + 1;
	writeWord(uint64_t(linux::AuxiliaryType::random));
	writeWord(blobPtr);
	blobPtr += 16;

	/* write the auxiliary vector null-entry out */
	writeWord(0);

	/* write the padding out */
	content.insert(content.end(), padding, 0);

	/* write the binary path out */
	writeBytes(pBinary.data(), pBinary.size() + 1);

	/* write the actual args to the blob */
	for (size_t i = 0; i < pArgs.size(); ++i)
		writeBytes(pArgs[i].data(), pArgs[i].size() + 1);

	/* write the actual envs to the blob */
	for (size_t i = 0; i < pEnvs.size(); ++i)
		writeBytes(pEnvs[i].data(), pEnvs[i].size() + 1);

	/* write the binary path out again (used for the auxiliary-vector) */
	writeBytes(pBinary.data(), pBinary.size() + 1);

	/* write the random bytes out */
	for (size_t i = 0; i < 4; ++i) {
		uint32_t tmp = host::Random();
		writeBytes(&tmp, 4);
	}

	/* write the prepared content to the acutal stack and return the pointer to
	*	the argument-count (ptr must not be zero, as this indicates failure) */
	env::guest_t stack = stackBase + Userspace::StackSize - totalSize;
	mem.mwrite(stack, content.data(), uint32_t(content.size()), env::Usage::Write);

	/* log the stack-state */
	for (size_t i = 0; i < content.size(); i += 8)
		logger.trace(u8"Stack [", str::As{ U"#018x", stack + i }, u8"] : ", str::As{ U"#018x", *(uint64_t*)(content.data() + i) });
	return stack;
}
void sys::Userspace::fStartLoad(const std::u8string& path) {
	/* load the binary */
	env::Instance()->filesystem().readStats(path, [this, path](const env::FileStats* stats) {
		/* check if the file could be resolved */
		if (stats == 0) {
			/* this error should be displayed to the user, no matter if logging is enabled or not */
			std::u8string msg = str::u8::Build(u8"Path [", path, u8"] does not exist");
			if (!logger.error(msg))
				host::PrintOut(msg);
			env::Instance()->shutdown();
			return;
		}

		/* check the file-type */
		if (stats->type != env::FileType::file) {
			/* this error should be displayed to the user, no matter if logging is enabled or not */
			std::u8string msg = str::u8::Build(u8"Path [", path, u8"] is not a file");
			if (!logger.error(msg))
				host::PrintOut(msg);
			env::Instance()->shutdown();
			return;
		}

		/* allocate the buffer for the file-content and read it into memory */
		uint8_t* buffer = new uint8_t[stats->size];
		env::Instance()->filesystem().readFile(stats->id, 0, buffer, stats->size, [this, size = stats->size, buffer](std::optional<uint64_t> read) {
			std::unique_ptr<uint8_t[]> _cleanup{ buffer };

			/* check if the size still matches */
			if (size != read) {
				/* this error should be displayed to the user, no matter if logging is enabled or not */
				std::u8string_view msg = (read.has_value() ? u8"Unable to read entire file" : u8"Error while reading file");
				if (!logger.error(msg))
					host::PrintOut(msg);
				env::Instance()->shutdown();
				return;
			}

			/* perform the actual loading of the file */
			if (!fBinaryLoaded(buffer, size))
				env::Instance()->shutdown();
			});
		});
}
bool sys::Userspace::fBinaryLoaded(const uint8_t* data, size_t size) {
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
			host::PrintOut(msg);
		return false;
	}

	/* validate the word-width (necessary, as env::Memory's allocate function currently
	*	allocates into the unreachable portion of the 32 bit address space) */
	if (pLoaded.bitWidth != 64) {
		/* this error should be displayed to the user, no matter if logging is enabled or not */
		std::u8string_view msg = u8"Only 64 bit elf binaries are supported";
		if (!logger.error(msg))
			host::PrintOut(msg);
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
			host::PrintOut(msg);
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
	if (!pSyscall.setup(this, pLoaded.endOfData, pBinary, fArchType(pCpu->architecture()))) {
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

bool sys::Userspace::Create(std::unique_ptr<sys::Cpu>&& cpu, const std::u8string& binary, const std::vector<std::u8string>& args, const std::vector<std::u8string>& envs, bool logBlocks, gen::TraceType trace, sys::Debugger** debugger) {
	uint32_t caches = cpu->memoryCaches(), context = cpu->contextSize();

	/* log the configuration */
	logger.info(u8"  Cpu         : [", cpu->name(), u8']');
	logger.info(u8"  Debug       : ", str::As{ U"S", (debugger != 0) });
	logger.info(u8"  Log Blocks  : ", str::As{ U"S", logBlocks });
	logger.info(u8"  Trace Blocks: ", trace);
	logger.info(u8"  Binary      : ", binary);
	logger.info(u8"  Arguments   : ", args.size());
	for (size_t i = 0; i < args.size(); ++i)
		logger.info(u8"    [", i, u8"]: ", args[i]);
	logger.info(u8"  Environments: ", envs.size());
	for (size_t i = 0; i < envs.size(); ++i)
		logger.info(u8"    [", i, u8"]: ", envs[i]);

	/* allocate the userspace object populate it */
	std::unique_ptr<sys::Userspace> system{ new sys::Userspace() };
	system->pArgs = args;
	system->pEnvs = envs;
	system->pBinary = binary;
	system->pCpu = cpu.get();

	/* check if the cpu can be set up */
	if (!system->pCpu->setupCpu(&system->pWriter)) {
		logger.error(u8"Failed to setup cpu [", cpu->name(), u8"] for userspace execution");
		return false;
	}
	if (system->pCpu->multiThreaded())
		logger.warn(u8"Userspace currently does not support true multiple threads");

	/* validate the cpu-architecture */
	if (cpu->architecture() == sys::ArchType::unknown) {
		logger.error(u8"Unsupported architecture [", system->fArchType(cpu->architecture()), u8"] used by cpu");
		return false;
	}

	/* check if the debugger could be created */
	sys::Debugger* debugObject = 0;
	std::function<void(env::guest_t)> debugCheck;
	if (debugger != 0) {
		/* setup the debugger itself */
		if (!system->pDebugger.fSetup(system.get())) {
			logger.error(u8"Failed to attach debugger");
			return false;
		}
		logger.info(u8"Debugger attached successfully");
		debugObject = &system->pDebugger;

		/* setup the debug-check function */
		debugCheck = [debugObject](env::guest_t address) { debugObject->fCheck(address); };
	}

	/* register the process and translator (translator first, as it will be used for core-creation) */
	bool detectWriteExecute = cpu->detectWriteExecute();
	if (!gen::SetInstance(std::move(cpu), Userspace::TranslationDepth, trace, debugCheck))
		return false;
	if (!env::SetInstance(std::move(system), Userspace::PageSize, caches, context, detectWriteExecute, logBlocks)) {
		gen::ClearInstance();
		return false;
	}

	/* check if a reference to the debugger should be returned */
	if (debugger != 0)
		*debugger = debugObject;
	return true;
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
	fStartLoad(pBinary);
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
