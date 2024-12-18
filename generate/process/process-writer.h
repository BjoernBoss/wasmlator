#pragma once

#include "../gen-common.h"
#include "process-builder.h"

namespace gen::detail {
	class ProcessWriter {
	private:
		const detail::ProcessState& pState;
		wasm::Sink& pSink;

	public:
		ProcessWriter(const detail::ProcessState& state, wasm::Sink& sink);

	public:
		void makeTerminate(env::guest_t address) const;
		void makeNotDecodable(env::guest_t address) const;
		void makeNotReachable(env::guest_t address) const;
		void makeSingleStep() const;
	};
}
