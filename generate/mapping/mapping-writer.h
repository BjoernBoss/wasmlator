#pragma once

#include "../gen-common.h"
#include "mapping-builder.h"

namespace gen::detail {
	class MappingWriter {
	private:
		const detail::MappingState& pState;

	public:
		MappingWriter(const detail::MappingState& state);

	public:
		void makeGetFunction() const;
		void makeDirectInvoke() const;
		void makeTailInvoke() const;
		const wasm::Prototype& blockPrototype() const;
	};
}
