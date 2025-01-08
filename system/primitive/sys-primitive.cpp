#include "../system.h"
#include "../../interface/random.h"

#include "../riscv-buffer.h"

static host::Logger logger{ u8"sys::primitive" };

env::guest_t sys::Primitive::fPrepareStack(const elf::Output& loaded) const {
	env::Memory& mem = env::Instance()->memory();
	size_t wordWidth = (loaded.is64Bit ? 8 : 4);

	/* count the size of the blob of data at the end (16 bytes for random data) */
	size_t blobSize = 16;
	for (size_t i = 0; i < pArgs.size(); ++i)
		blobSize += pArgs[i].size() + 1;
	for (size_t i = 0; i < pEnvs.size(); ++i)
		blobSize += pEnvs[i].size() + 1;

	/* compute the size of the structured stack-data
	*	Args: one pointer for each argument plus count plus null-entry
	*	Envs: one pointer for each environment plus null-entry
	*	Auxiliary: (AT_PHDR/AT_PHNUM/AT_PHENT/AT_ENTRY/AT_RANDOM/AT_PAGESZ) one null-entry */
	size_t structSize = 0;
	structSize += wordWidth * (pArgs.size() + 2);
	structSize += wordWidth * (pEnvs.size() + 1);
	structSize += wordWidth * 12 + wordWidth;

	/* validate the stack dimensions (aligned to the corresponding boundaries) */
	size_t totalSize = (blobSize + structSize + Primitive::StartOfStackAlignment - 1) & ~(Primitive::StartOfStackAlignment - 1);
	size_t padding = totalSize - (blobSize + structSize);
	if (totalSize > Primitive::StackSize) {
		logger.error(u8"Cannot fit [", totalSize, u8"] bytes onto the initial stack of size [", Primitive::StackSize, u8']');
		return 0;
	}

	/* allocate the new stack */
	env::guest_t stackBase = mem.alloc(Primitive::StackSize, env::Usage::ReadWrite);
	if (stackBase == 0) {
		logger.error(u8"Failed to allocate initial stack");
		return 0;
	}
	logger.debug(u8"Stack allocated to [", str::As{ U"#018x", stackBase }, u8"] with size [", str::As{ U"#010x", Primitive::StackSize }, u8']');

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
	env::guest_t blobPtr = stackBase + Primitive::StackSize - blobSize;

	/* write the argument-count out */
	writeWord(pArgs.size());

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
	writeWord(uint64_t(elf::AuxiliaryType::phAddress));
	writeWord(loaded.phAddress);
	writeWord(uint64_t(elf::AuxiliaryType::phCount));
	writeWord(loaded.phCount);
	writeWord(uint64_t(elf::AuxiliaryType::phEntrySize));
	writeWord(loaded.phEntrySize);
	writeWord(uint64_t(elf::AuxiliaryType::entry));
	writeWord(loaded.start);
	writeWord(uint64_t(elf::AuxiliaryType::random));
	writeWord(blobPtr);
	blobPtr += 16;
	writeWord(uint64_t(elf::AuxiliaryType::pageSize));
	writeWord(env::Instance()->pageSize());

	/* write the auxiliary vector null-entry out */
	writeWord(0);

	/* write the padding out */
	content.insert(content.end(), padding, 0);

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
	env::guest_t stack = stackBase + Primitive::StackSize - totalSize;
	mem.mwrite(stack, content.data(), uint32_t(content.size()), env::Usage::Write);
	return stack;
}
void sys::Primitive::fExecute() {
	/* start execution of the next address and catch/handle any incoming exceptions */
	try {
		do {
			pAddress = env::Instance()->mapping().execute(pAddress);
		} while (pDebugger.get() == 0 || pDebugger->advance(pAddress));
	}
	catch (const env::Terminated& e) {
		pAddress = e.address;
		logger.log(u8"Execution terminated at [", str::As{ U"#018x", e.address }, u8"] with", e.code);
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
		if (pDebugger.get() == 0)
			logger.debug(u8"Translate caught: [", str::As{ U"#018x", e.address }, u8']');
		env::Instance()->startNewBlock();
	}
	catch (const detail::FlushInstCache& e) {
		pAddress = e.address;
		logger.debug(u8"Flushing instruction cache");
		env::Instance()->mapping().flush();

		/* check if the execution should halt */
		if (pDebugger.get() == 0 || pDebugger->advance(pAddress))
			env::Instance()->startNewBlock();
	}
	catch (const detail::CpuException& e) {
		pAddress = e.address;
		logger.fatal(u8"CPU Exception caught: [", str::As{ U"#018x", e.address }, u8"] - [", pCpu->getExceptionText(e.id), u8']');
	}
	catch (const sys::UnknownSyscall& e) {
		pAddress = e.address;
		logger.fatal(u8"Unknown syscall caught: [", str::As{ U"#018x", e.address }, u8"] - [Index: ", e.index, u8']');
	}
}

bool sys::Primitive::Create(std::unique_ptr<sys::Cpu>&& cpu, const std::vector<std::u8string>& args, const std::vector<std::u8string>& envs, bool debug, bool logBlocks, bool traceBlocks, sys::Debugger** debugger) {
	uint32_t caches = cpu->memoryCaches(), context = cpu->contextSize();

	/* log the configuration */
	logger.info(u8"  Cpu         : [", cpu->name(), u8']');
	logger.info(u8"  Debug       : ", str::As{ U"S", debug });
	logger.info(u8"  Log Blocks  : ", str::As{ U"S", logBlocks });
	logger.info(u8"  Trace Blocks: ", str::As{ U"S", traceBlocks });
	logger.info(u8"  Arguments   : ", args.size());
	for (size_t i = 0; i < args.size(); ++i)
		logger.info(u8"    [", i, u8"]: ", args[i]);
	logger.info(u8"  Environments: ", envs.size());
	for (size_t i = 0; i < envs.size(); ++i)
		logger.info(u8"    [", i, u8"]: ", envs[i]);

	/* allocate the primitive object populate it */
	std::unique_ptr<sys::Primitive> system = std::make_unique<sys::Primitive>();
	sys::Primitive* self = system.get();
	std::unique_ptr<detail::PrimitiveExecContext> execContext = detail::PrimitiveExecContext::New(self);
	self->pArgs = args;
	self->pEnvs = envs;
	self->pCpu = cpu.get();
	self->pDebugger = (debug ? sys::Debugger::New(detail::PrimitiveDebugger::New(self)) : 0);
	self->pExecContext = execContext.get();

	/* check if the debugger could be created */
	if (debug && self->pDebugger.get() == 0) {
		logger.error(u8"Failed to attach debugger (supported by cpu?)");
		return false;
	}

	/* construct the new cpu object */
	if (!cpu->setupCpu(std::move(execContext)))
		return false;

	/* register the process and translator (translator first, as it will be used for core-creation) */
	if (!gen::SetInstance(std::move(cpu), Primitive::TranslationDepth, debug, traceBlocks))
		return false;
	if (!env::SetInstance(std::move(system), Primitive::PageSize, caches, context, logBlocks)) {
		gen::ClearInstance();
		return false;
	}

	/* check if a reference to the debugger should be returned */
	if (debugger != 0)
		*debugger = self->pDebugger.get();
	return true;
}

bool sys::Primitive::setupCore(wasm::Module& mod) {
	/* setup the actual core */
	gen::Core core{ mod };

	/* perform the cpu-configuration of the core */
	if (!pCpu->setupCore(mod))
		return false;

	/* finalize the actual core */
	return core.close();
}
bool sys::Primitive::coreLoaded() {
	/* load the static elf-image and configure the startup-address */
	elf::Output loaded;
	try {
		loaded = elf::LoadStatic(fileBuffer, sizeof(fileBuffer));
		logger.debug(u8"Start of program: ", str::As{ U"#018x", loaded.start });
		logger.debug(u8"Start of heap   : ", str::As{ U"#018x", loaded.endOfData });
		pAddress = loaded.start;
	}
	catch (const elf::Exception& e) {
		logger.error(u8"Failed to load static elf: ", e.what());
		return false;
	}

	/* validate the word-width (necessary, as env::Memory's allocate function currently
	*	allocates into the unreachable portion of the 32 bit address space) */
	if (!loaded.is64Bit) {
		logger.error(u8"Only 64 bit static elf binaries are supported");
		return false;
	}

	/* initialize the stack based on the system-v ABI stack specification (architecture independent) */
	env::guest_t spAddress = fPrepareStack(loaded);
	if (spAddress == 0)
		return false;
	logger.debug(u8"Stack loaded to: ", str::As{ U"#018x", spAddress });

	/* initialize the context */
	if (!pCpu->setupContext(pAddress, spAddress))
		return false;

	/* initialize the execution-context */
	if (!pExecContext->coreLoaded(loaded.endOfData))
		return false;

	/* check if this is debug-mode, in which case no block needs to be translated
	*	yet, otherwise immediately startup the translation and execution */
	if (pDebugger.get() == 0)
		env::Instance()->startNewBlock();
	return true;
}
std::vector<env::BlockExport> sys::Primitive::setupBlock(wasm::Module& mod) {
	/* setup the translator */
	gen::Block translator{ mod };

	/* translate the next requested address */
	translator.run(pAddress);

	/* finalize the translation */
	return translator.close();
}
void sys::Primitive::blockLoaded() {
	fExecute();
}
void sys::Primitive::shutdown() {
	/* clearing the system-reference will also release this object */
	gen::ClearInstance();
	env::ClearInstance();
}
