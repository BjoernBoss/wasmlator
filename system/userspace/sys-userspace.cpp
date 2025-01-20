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
env::guest_t sys::Userspace::fPrepareStack(const sys::ElfLoaded& loaded, bool is64Bit) const {
	env::Memory& mem = env::Instance()->memory();
	size_t wordWidth = (is64Bit ? 8 : 4);

	/* count the size of the blob of data at the end (16 bytes for random data) */
	size_t blobSize = 16 + pBinary.size() + 1;
	for (size_t i = 0; i < pArgs.size(); ++i)
		blobSize += pArgs[i].size() + 1;
	for (size_t i = 0; i < pEnvs.size(); ++i)
		blobSize += pEnvs[i].size() + 1;

	/* compute the size of the structured stack-data
	*	Args: one pointer for each argument plus count plus null-entry plus self-path
	*	Envs: one pointer for each environment plus null-entry
	*	Auxiliary: (AT_PHDR/AT_PHNUM/AT_PHENT/AT_ENTRY/AT_RANDOM/AT_PAGESZ) one null-entry */
	size_t structSize = 0;
	structSize += wordWidth * (pArgs.size() + 3);
	structSize += wordWidth * (pEnvs.size() + 1);
	structSize += wordWidth * 12 + wordWidth;

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
	writeWord(loaded.phAddress);
	writeWord(uint64_t(linux::AuxiliaryType::phCount));
	writeWord(loaded.phCount);
	writeWord(uint64_t(linux::AuxiliaryType::phEntrySize));
	writeWord(loaded.phEntrySize);
	writeWord(uint64_t(linux::AuxiliaryType::entry));
	writeWord(loaded.start);
	writeWord(uint64_t(linux::AuxiliaryType::random));
	writeWord(blobPtr);
	blobPtr += 16;
	writeWord(uint64_t(linux::AuxiliaryType::pageSize));
	writeWord(env::Instance()->pageSize());

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

	/* write the random bytes out */
	for (size_t i = 0; i < 4; ++i) {
		uint32_t tmp = host::Random();
		writeBytes(&tmp, 4);
	}

	/* write the prepared content to the acutal stack and return the pointer to
	*	the argument-count (ptr must not be zero, as this indicates failure) */
	env::guest_t stack = stackBase + Userspace::StackSize - totalSize;
	mem.mwrite(stack, content.data(), uint32_t(content.size()), env::Usage::Write);
	return stack;
}
bool sys::Userspace::fBinaryLoaded(const uint8_t* data, size_t size) {
	/* validate the initial elf-header */
	if (!elf::CheckValid(data, size)) {
		logger.error(u8"Binary [", pBinary, u8"] is not a valid elf binary");
		return false;
	}

	/* validate the word-width (necessary, as env::Memory's allocate function currently
	*	allocates into the unreachable portion of the 32 bit address space) */
	if (elf::GetBitWidth(data, size) != 64) {
		logger.error(u8"Only 64 bit elf binaries are supported");
		return false;
	}
	bool is64Bit = true;

	/* validate the machine type and match it */
	sys::ArchType arch = sys::ArchType::unknown;
	switch (elf::GetMachine(data, size)) {
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
		logger.error(u8"Cpu for architecture [", fArchType(pCpu->architecture()), u8"] cannot execute elf of type [", fArchType(arch), u8']');
		return false;
	}

	/* check the elf-kind */
	elf::ElfType elfType = elf::GetType(data, size);
	if (elfType != elf::ElfType::executable && elfType != elf::ElfType::dynamic) {
		logger.error(u8"Elf is not an executable");
		return false;
	}

	/* load the static elf-image and configure the startup-address */
	sys::ElfLoaded loaded;
	try {
		/* load the static elf */
		sys::LoadElf(data, size);
		logger.debug(u8"Start of program: ", str::As{ U"#018x", loaded.start });
		logger.debug(u8"Start of heap   : ", str::As{ U"#018x", loaded.endOfData });
		pAddress = loaded.start;
	}
	catch (const elf::Exception& e) {
		logger.error(u8"Failed to load static elf: ", e.what());
		return false;
	}
	catch (const elf::Interpreter& e) {
		/* check if this was already the interpreter */
		if (!pInterpreter.empty()) {
			logger.error(u8"Recursive interpreter requirements encountered");
			return false;
		}
		pInterpreter = e.path();

		/* load the interpreter */
		logger.info(u8"Binary requires interpreter [", pInterpreter, u8']');
		fStartLoad(pInterpreter);
		return true;
	}

	/* initialize the stack based on the system-v ABI stack specification (architecture independent) */
	env::guest_t spAddress = fPrepareStack(loaded, is64Bit);
	if (spAddress == 0)
		return false;
	logger.debug(u8"Stack loaded to: ", str::As{ U"#018x", spAddress });

	/* initialize the syscall environment */
	if (!pSyscall.setup(this, loaded.endOfData, pBinary, fArchType(pCpu->architecture()))) {
		logger.error(u8"Failed to setup the userspace syscalls");
		return false;
	}

	/* initialize the context */
	if (!pCpu->setupContext(pAddress, spAddress))
		return false;

	/* log the system as fully loaded */
	logger.log(u8"Userspace environment fully initialized");

	/* check if this is debug-mode, in which case no block needs to be translated
	*	yet, otherwise immediately startup the translation and execution */
	if (!pDebugger.fActive())
		env::Instance()->startNewBlock();
	return true;
}
void sys::Userspace::fExecute() {
	/* start execution of the next address and catch/handle any incoming exceptions */
	try {
		do {
			pAddress = env::Instance()->mapping().execute(pAddress);
		} while (pDebugger.fAdvance(pAddress));
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
		logger.fmtFatal(u8"MemoryFault detected at: [{:#018x}] while accessing [{:#018x}] with attributes [{:03b}] while page is mapped as [{:03b}]",
			e.address, e.accessed, e.usedUsage, e.actualUsage);
	}
	catch (const env::Decoding& e) {
		pAddress = e.address;
		logger.fatal(u8"Decoding caught: [", str::As{ U"#018x", e.address }, u8"] - [", (e.memoryFault ? u8"Memory-Fault" : u8"Decoding-Fault"), u8']');
	}
	catch (const env::Translate& e) {
		pAddress = e.address;
		if (!pDebugger.fActive())
			logger.debug(u8"Translate caught: [", str::As{ U"#018x", e.address }, u8']');
		env::Instance()->startNewBlock();
	}
	catch (const detail::FlushInstCache& e) {
		pAddress = e.address;
		logger.debug(u8"Flushing instruction cache");
		env::Instance()->mapping().flush();

		/* check if the execution should halt */
		if (pDebugger.fAdvance(pAddress))
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
	catch (const detail::AwaitingSyscall& e) {
		pAddress = e.address;
		logger.trace(u8"Awaiting syscall result from [", str::As{ U"#018x", e.address }, u8']');
	}
}
void sys::Userspace::fStartLoad(const std::u8string& path) {
	/* load the binary */
	env::Instance()->filesystem().readStats(path, [this, path](const env::FileStats* stats) {
		/* check if the file could be resolved */
		if (stats == 0) {
			logger.error(u8"Path [", path, u8"] does not exist");
			env::Instance()->shutdown();
			return;
		}

		/* check the file-type */
		if (stats->type != env::FileType::file) {
			logger.error(u8"Path [", path, u8"] is not a file");
			env::Instance()->shutdown();
			return;
		}

		/* setup the opening of the file */
		env::Instance()->filesystem().openFile(path, env::FileOpen::openExisting, env::FileAccess{}, [this, path](bool success, uint64_t id, const env::FileStats* stats) {
			/* check if the file could be opened */
			if (!success) {
				logger.error(u8"Failed to open file [", path, u8"] for reading");
				env::Instance()->shutdown();
				return;
			}

			/* allocate the buffer for the file-content and read it into memory */
			uint64_t size = stats->size;
			uint8_t* buffer = new uint8_t[size];
			env::Instance()->filesystem().readFile(id, 0, buffer, stats->size, [this, buffer, size, id](uint64_t count) {
				std::unique_ptr<uint8_t[]> _cleanup{ buffer };

				/* close the file again, as no more data will be read */
				env::Instance()->filesystem().closeFile(id);

				/* check if the size still matches */
				if (count != size) {
					logger.error(u8"File size does not match expected size");
					env::Instance()->shutdown();
					return;
				}

				/* perform the actual loading of the file */
				if (!fBinaryLoaded(buffer, size))
					env::Instance()->shutdown();
				});
			});
		});
}

bool sys::Userspace::Create(std::unique_ptr<sys::Cpu>&& cpu, const std::u8string& binary, const std::vector<std::u8string>& args, const std::vector<std::u8string>& envs, bool logBlocks, bool traceBlocks, sys::Debugger** debugger) {
	uint32_t caches = cpu->memoryCaches(), context = cpu->contextSize();

	/* log the configuration */
	logger.info(u8"  Cpu         : [", cpu->name(), u8']');
	logger.info(u8"  Debug       : ", str::As{ U"S", (debugger != 0) });
	logger.info(u8"  Log Blocks  : ", str::As{ U"S", logBlocks });
	logger.info(u8"  Trace Blocks: ", str::As{ U"S", traceBlocks });
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
	if (debugger != 0) {
		if (system->pDebugger.fSetup(system.get())) {
			logger.info(u8"Debugger attached successfully");
			debugObject = &system->pDebugger;
		}
		else {
			logger.warn(u8"Failed to attach debugger");
			*debugger = 0;
			debugger = 0;
		}
	}

	/* register the process and translator (translator first, as it will be used for core-creation) */
	if (!gen::SetInstance(std::move(cpu), Userspace::TranslationDepth, (debugger != 0), traceBlocks))
		return false;
	if (!env::SetInstance(std::move(system), Userspace::PageSize, caches, context, logBlocks)) {
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
