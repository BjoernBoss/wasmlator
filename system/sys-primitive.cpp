#include "sys-primitive.h"

#include "elf/elf-static.h"
#include "../riscv-buffer.h"

static host::Logger logger{ u8"sys::primitive" };

sys::detail::PrimitiveExecContext::PrimitiveExecContext(sys::Primitive* primitive) : sys::ExecContext{ false, true }, pPrimitive{ primitive } {}
std::unique_ptr<sys::ExecContext> sys::detail::PrimitiveExecContext::New(sys::Primitive* primitive) {
	return std::unique_ptr<detail::PrimitiveExecContext>(new detail::PrimitiveExecContext{ primitive });
}
void sys::detail::PrimitiveExecContext::syscall(env::guest_t address, env::guest_t nextAddress) {
	gen::Make->invokeVoid(pPrimitive->pRegistered.syscall);
}
void sys::detail::PrimitiveExecContext::throwException(uint64_t id, env::guest_t address, env::guest_t nextAddress) {
	gen::Make->invokeVoid(pPrimitive->pRegistered.exception);
}
void sys::detail::PrimitiveExecContext::flushMemCache(env::guest_t address, env::guest_t nextAddress) {
	/* nothing to be done here, as the system is considered single-threaded */
}
void sys::detail::PrimitiveExecContext::flushInstCache(env::guest_t address, env::guest_t nextAddress) {
	gen::Add[I::U64::Const(pPrimitive->pRegistered.flushInst)];
	gen::Make->invokeParam(pPrimitive->pRegistered.flushInst);
	gen::Add[I::Drop()];
}


env::guest_t sys::Primitive::fPrepareStack() const {
	env::Memory& mem = env::Instance()->memory();

	/* count the size of the blob of data at the end */
	size_t blobSize = 0;
	for (size_t i = 0; i < pArgs.size(); ++i)
		blobSize += pArgs[i].size() + 1;
	for (size_t i = 0; i < pEnvs.size(); ++i)
		blobSize += pEnvs[i].size() + 1;

	/* compute the size of the structured stack-data
	*	Args: one pointer for each argument plus count plus null-entry
	*	Envs: one pointer for each environment plus null-entry
	*	Auxiliary: one null-entry */
	size_t structSize = 0;
	structSize += sizeof(uint64_t) * (pArgs.size() + 2);
	structSize += sizeof(uint64_t) * (pEnvs.size() + 1);
	structSize += sizeof(uint64_t);

	/* validate the stack dimensions (aligned to the corresponding boundaries) */
	size_t totalSize = (blobSize + structSize + Primitive::StartOfStackAlignment - 1) & ~(Primitive::StartOfStackAlignment - 1);
	size_t padding = totalSize - (blobSize + structSize);
	if (totalSize > Primitive::StackSize)
		logger.fatal(u8"Cannot fit [", totalSize, u8"] bytes onto the initial stack of size [", Primitive::StackSize, u8']');

	/* allocate the new stack and initialize the content for it */
	if (!mem.mmap(Primitive::InitialStackAddress, Primitive::StackSize, env::Usage::ReadWrite))
		logger.fatal(u8"Failed to allocate initial stack");
	std::vector<uint8_t> content;
	auto write = [&](const void* data, size_t size) {
		const uint8_t* d = static_cast<const uint8_t*>(data);
		content.insert(content.end(), d, d + size);
		};
	uint64_t blobPtr = Primitive::InitialStackAddress + Primitive::StackSize - blobSize;

	/* write the argument-count out */
	uint64_t count = pArgs.size(), nullValue = 0;
	write(&count, sizeof(count));

	/* write the argument pointers out and the trailing null-entry */
	for (size_t i = 0; i < pArgs.size(); ++i) {
		write(&blobPtr, sizeof(blobPtr));
		blobPtr += pArgs[i].size() + 1;
	}
	write(&nullValue, sizeof(nullValue));

	/* write the environment pointers out and the trailing null-entry */
	for (size_t i = 0; i < pEnvs.size(); ++i) {
		write(&blobPtr, sizeof(blobPtr));
		blobPtr += pEnvs[i].size() + 1;
	}
	write(&nullValue, sizeof(nullValue));

	/* write the empty auxiliary vector out */
	write(&nullValue, sizeof(nullValue));

	/* write the padding out */
	content.insert(content.end(), padding, 0);

	/* write the actual args to the blob */
	for (size_t i = 0; i < pArgs.size(); ++i)
		write(pArgs[i].data(), pArgs[i].size() + 1);

	/* write the actual envs to the blob */
	for (size_t i = 0; i < pEnvs.size(); ++i)
		write(pEnvs[i].data(), pEnvs[i].size() + 1);

	/* write the prepared content to the acutal stack and return the pointer to the argument-count */
	env::guest_t stack = Primitive::InitialStackAddress + Primitive::StackSize - totalSize;
	mem.mwrite(stack, content.data(), uint32_t(content.size()), env::Usage::Write);
	return stack;
}
bool sys::Primitive::Create(std::unique_ptr<sys::Cpu>&& cpu, const std::vector<std::u8string>& args, const std::vector<std::u8string>& envs, bool debug) {
	uint32_t caches = cpu->memoryCaches(), context = cpu->contextSize();

	/* log the configuration */
	logger.info(u8"  Debug       : ", str::As{ U"S", debug });
	logger.info(u8"  Arguments   : ", args.size());
	for (size_t i = 0; i < args.size(); ++i)
		logger.info(u8"    [", i, u8"]: ", args[i]);
	logger.info(u8"  Environments: ", envs.size());
	for (size_t i = 0; i < envs.size(); ++i)
		logger.info(u8"    [", i, u8"]: ", envs[i]);

	/* allocate the primitive object and configure the cpu itself */
	std::unique_ptr<sys::Primitive> system = std::make_unique<sys::Primitive>();
	cpu->setupCpu(std::move(detail::PrimitiveExecContext::New(system.get())));

	/* construct the primitive env-system object */
	system->pArgs = args;
	system->pEnvs = envs;

	/* register the process and translator (translator first, as it will be used for core-creation) */
	if (!gen::SetInstance(std::move(cpu), Primitive::TranslationDepth, debug))
		return false;
	if (env::SetInstance(std::move(system), Primitive::PageSize, caches, context, debug))
		return true;
	gen::ClearInstance();
	return false;
}
void sys::Primitive::setupCore(wasm::Module& mod) {
	/* setup the actual core */
	gen::Core core{ mod };

	/* perform the cpu-configuration of the core */
	dynamic_cast<sys::Cpu*>(gen::Instance()->translator())->setupCore(mod);

	/* finalize the actual core */
	core.close();
}
std::vector<env::BlockExport> sys::Primitive::setupBlock(wasm::Module& mod) {
	/* setup the translator */
	gen::Block translator{ mod };

	/* translate the next requested address */
	translator.run(pAddress);

	/* finalize the translation */
	return translator.close();
}
void sys::Primitive::coreLoaded() {
	/* load the static elf-image and configure the startup-address */
	try {
		pAddress = elf::LoadStatic(fileBuffer, sizeof(fileBuffer));
	}
	catch (const elf::Exception& e) {
		logger.fatal(u8"Failed to load static elf: ", e.what());
	}

	/* initialize the stack based on the system-v ABI stack specification (architecture independent) */
	env::guest_t spAddress = fPrepareStack();

	/* register the functions to be invoked by the execution-environment */
	pRegistered.flushInst = env::Instance()->interact().defineCallback([](uint64_t address) -> uint64_t {
		throw detail::FlushInstCache{ address };
		return 0;
		});
	pRegistered.syscall = env::Instance()->interact().defineCallback([]() {
		logger.fatal(u8"Syscall invoked");
		});
	pRegistered.exception = env::Instance()->interact().defineCallback([]() {
		logger.fatal(u8"Exception caught");
		});

	/* initialize the context */
	dynamic_cast<sys::Cpu*>(gen::Instance()->translator())->setupContext(pAddress, spAddress);

	/* request the translation of the first block */
	env::Instance()->startNewBlock();
}
void sys::Primitive::blockLoaded() {
	/* start execution of the next address and catch/handle any incoming exceptions */
	try {
		env::Instance()->mapping().execute(pAddress);
	}
	catch (const env::Terminated& e) {
		logger.log(u8"Execution terminated at [", str::As{ U"#018x", e.address }, u8"] with", e.code);
	}
	catch (const env::MemoryFault& e) {
		logger.fmtFatal(u8"MemoryFault detected at: [{:#018x}] while accessing [{:#018x}] with attributes [{:03b}] while page is mapped as [{:03b}]",
			e.address, e.accessed, e.usedUsage, e.actualUsage);
	}
	catch (const env::NotDecodable& e) {
		logger.fatal(u8"NotDecodable caught: [", str::As{ U"#018x", e.address }, u8']');
	}
	catch (const env::Translate& e) {
		logger.debug(u8"Translate caught: [", str::As{ U"#018x", e.address }, u8']');
		pAddress = e.address;
		env::Instance()->startNewBlock();
	}
}
