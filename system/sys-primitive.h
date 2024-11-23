#pragma once

#include "../environment/environment.h"
#include "../generate/generate.h"

#include "sys-cpu.h"

namespace sys {
	/* primitive system, which set up an environment, loads the code to
	*	be executed, and passes the calls to the cpu implementation */
	class Primitive final : public env::System {
	private:
		static constexpr uint32_t TranslationDepth = 4;
		static constexpr uint32_t PageSize = 0x1000;

	private:
		std::unique_ptr<sys::Cpu> pCpu;
		env::guest_t pAddress = 0;

	private:
		Primitive(std::unique_ptr<sys::Cpu>&& cpu, uint32_t memoryCaches, uint32_t contextSize);

	public:
		static std::unique_ptr<env::System> New(std::unique_ptr<sys::Cpu>&& cpu);

	public:
		void setupCore(wasm::Module& mod) final;
		std::vector<env::BlockExport> setupBlock(wasm::Module& mod) final;
		void coreLoaded() final;
		void blockLoaded() final;
	};
}
