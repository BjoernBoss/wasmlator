#pragma once

#include "../gen-common.h"
#include "../interact/interact-builder.h"
#include "../process/process-builder.h"

namespace gen {
	/* env::Process instance must be created
	*	Note: will perform imports as well as initial definitions
	*	Note: close must be called explicitly, and afterwards nothing must be done to the configurations of env::Process (module will be closed internally) */
	class Core {
	private:
		detail::InteractBuilder pInteract;
		detail::ProcessBuilder pProcess;
		bool pClosed = false;

	public:
		Core(wasm::Module& mod);

	private:
		void fClose();

	public:
		void close();
	};
}
