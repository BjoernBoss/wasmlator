#pragma once

#include "../trans-common.h"
#include "mapping-builder.h"

namespace trans::detail {
	class MappingWriter {
	private:
		env::Process* pProcess = 0;
		detail::MappingState pState;
		wasm::Sink& pSink;

	public:
		void makeLookup() const;
		void makeInvoke() const;
	};
}
