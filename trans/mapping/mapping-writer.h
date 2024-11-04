#pragma once

#include "../trans-common.h"
#include "mapping-builder.h"

namespace trans::detail {
	class MappingWriter {
	private:
		detail::MappingState pState;
		env::Process* pProcess = 0;
		wasm::Sink& pSink;

	public:
		MappingWriter(const detail::MappingState& state, env::Process* process, wasm::Sink& sink);

	public:
		void makeLookup() const;
		void makeLoadFunction() const;
		void makeInvoke() const;
	};
}
