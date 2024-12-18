#include "sys-primitive.h"

#include "elf/elf-static.h"
#include "riscv-buffer.h"

static host::Logger logger{ u8"sys::primitive" };

namespace I = wasm::inst;

sys::detail::PrimitiveExecContext::PrimitiveExecContext(sys::Primitive* primitive) : sys::ExecContext{ false, true }, pPrimitive{ primitive } {}
std::unique_ptr<sys::ExecContext> sys::detail::PrimitiveExecContext::New(sys::Primitive* primitive) {
	return std::unique_ptr<detail::PrimitiveExecContext>(new detail::PrimitiveExecContext{ primitive });
}
void sys::detail::PrimitiveExecContext::syscall(const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) {
	writer.invokeVoid(pPrimitive->pRegistered.syscall);
}
void sys::detail::PrimitiveExecContext::throwException(uint64_t id, const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) {
	writer.invokeVoid(pPrimitive->pRegistered.exception);
}
void sys::detail::PrimitiveExecContext::flushMemCache(const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) {
	/* nothing to be done here, as the system is considered single-threaded */
}
void sys::detail::PrimitiveExecContext::flushInstCache(const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) {
	writer.sink()[I::U64::Const(pPrimitive->pRegistered.flushInst)];
	writer.invokeParam(pPrimitive->pRegistered.flushInst);
	writer.sink()[I::Drop()];
}


sys::Primitive::Primitive(uint32_t memoryCaches, uint32_t contextSize) : env::System{ Primitive::PageSize, memoryCaches, contextSize } {}
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
std::unique_ptr<env::System> sys::Primitive::New(std::unique_ptr<sys::Cpu>&& cpu, const std::vector<std::u8string>& args, const std::vector<std::u8string>& envs) {
	uint32_t caches = cpu->memoryCaches(), context = cpu->contextSize();

	/* allocate the primitive object and configure the cpu itself */
	std::unique_ptr<sys::Primitive> system = std::unique_ptr<sys::Primitive>(new sys::Primitive{ caches, context });
	cpu->setupCpu(std::move(detail::PrimitiveExecContext::New(system.get())));

	/* construct the primitive env-system object */
	system->pCpu = std::move(cpu);
	system->pArgs = args;
	system->pEnvs = envs;
	return system;
}
void sys::Primitive::setupCore(wasm::Module& mod) {
	/* setup the actual core */
	gen::Core core{ mod };

	/* perform the cpu-configuration of the core */
	pCpu->setupCore(mod);

	/* finalize the actual core */
	core.close();
}
std::vector<env::BlockExport> sys::Primitive::setupBlock(wasm::Module& mod) {
	/* setup the translator */
	gen::Block translator{ mod, pCpu.get() };

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
	pCpu->setupContext(pAddress, spAddress);

	/* request the translation of the first block */
	env::Instance()->startNewBlock();
}
void sys::Primitive::blockLoaded() {
	/* start execution of the next address and catch/handle any incoming exceptions */
	try {
		env::Instance()->mapping().execute(pAddress);
	}
	catch (const env::Terminated& e) {
		logger.fatal(u8"Execution terminated at [", str::As{ U"#018x", e.address }, u8"] with", e.code);
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
