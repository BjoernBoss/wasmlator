#include "sys-primitive.h"

#include "elf/elf-static.h"
#include "riscv-buffer.h"

sys::Primitive::Primitive(std::unique_ptr<sys::Cpu>&& cpu, uint32_t memoryCaches, uint32_t contextSize) : env::System{ PageSize, memoryCaches, contextSize }, pCpu{ std::move(cpu) } {}

std::unique_ptr<env::System> sys::Primitive::New(std::unique_ptr<sys::Cpu>&& cpu) {
	uint32_t caches = cpu->memoryCaches(), context = cpu->contextSize();

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
		host::Fatal(u8"Failed to load static elf: ", e.what());
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
		host::Fatal(str::u8::Format(u8"Execution terminated at {:#018x} with {}", e.address, e.code));
	}
	catch (const env::MemoryFault& e) {
		host::Fatal(str::u8::Format(u8"MemoryFault detected at: {:#018x}", e.address));
	}
	catch (const env::NotDecodable& e) {
		host::Fatal(str::u8::Format(u8"NotDecodable caught: {:#018x}", e.address));
	}
	catch (const env::Translate& e) {
		host::Debug(str::u8::Format(u8"Translate caught: {:#018x}", e.address));
		pAddress = e.address;
		env::Instance()->startNewBlock();
	}
}
