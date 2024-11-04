#pragma once

#include "../trans-common.h"
#include "trans-address.h"
#include "../mapping/mapping-writer.h"

namespace trans::detail {
	class AddressWriter {
	private:
		detail::MappingWriter pMapping;
		detail::Addresses& pHost;
		wasm::Sink& pSink;
		mutable wasm::Variable pTempFuncIndex;
		mutable wasm::Variable pTempRetState;
		mutable wasm::Variable pTempRetAddress;

	public:
		AddressWriter(const detail::MappingState& mapping, detail::Addresses& host, wasm::Sink& sink);

	private:
		void fCallLandingPad(env::guest_t nextAddress) const;

	public:
		void makeCall(env::guest_t address, env::guest_t nextAddress) const;
		void makeCallIndirect(env::guest_t nextAddress) const;
		void makeJump(env::guest_t address) const;
		void makeJumpIndirect() const;
		void makeReturn() const;
	};
}
