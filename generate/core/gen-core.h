#pragma once

#include "../gen-common.h"
#include "../interact/interact-builder.h"
#include "../process/process-builder.h"

namespace gen {
	/* env::Process instance must be created
	*	Note: will perform imports as well as initial definitions
	*	Note: after close/destructor, nothing must be done to the module or configurations to env::Process (will close the module as well) */
	class Core {
	private:
		wasm::Module& pModule;
		detail::InteractBuilder pInteract;
		detail::ProcessBuilder pProcess;
		bool pClosed = false;

	public:
		Core(wasm::Module& mod);
		~Core() noexcept(false);

	private:
		void fClose();

	public:
		void close();
	};
}
