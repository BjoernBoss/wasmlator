#include "sys-primitive.h"

#include "elf/elf-static.h"
#include "riscv-buffer.h"

static host::Logger logger{ u8"sys::primitive" };

sys::detail::PrimitiveExecContext::PrimitiveExecContext() : sys::ExecContext{ false, true } {}
std::unique_ptr<sys::ExecContext> sys::detail::PrimitiveExecContext::New() {
	return std::unique_ptr<detail::PrimitiveExecContext>(new detail::PrimitiveExecContext{});
}
void sys::detail::PrimitiveExecContext::syscall(const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) {
}
void sys::detail::PrimitiveExecContext::throwException(uint64_t id, const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) {
}
void sys::detail::PrimitiveExecContext::flushMemCache(const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) {
}
void sys::detail::PrimitiveExecContext::flushInstCache(const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) {
}


sys::Primitive::Primitive(std::unique_ptr<sys::Cpu>&& cpu, uint32_t memoryCaches, uint32_t contextSize) : env::System{ PageSize, memoryCaches, contextSize }, pCpu{ std::move(cpu) } {}
std::unique_ptr<env::System> sys::Primitive::New(std::unique_ptr<sys::Cpu>&& cpu) {
	uint32_t caches = cpu->memoryCaches(), context = cpu->contextSize();

	/* configure the cpu itself */
	cpu->setupCpu(std::move(detail::PrimitiveExecContext::New()));

	/* construct the primitive env-system object */
	return std::unique_ptr<sys::Primitive>(new sys::Primitive{ std::move(cpu), caches, context });
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
	gen::Block translator{ mod, pCpu.get(), TranslationDepth };

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

	/* initialize the context */
	pCpu->setupContext(pAddress);

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
		logger.fatal(u8"MemoryFault detected at: [", str::As{ U"#018x", e.address }, u8']');
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
