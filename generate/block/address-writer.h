#pragma once

#include "../gen-common.h"
#include "gen-address.h"
#include "../mapping/mapping-writer.h"
#include "../process/process-writer.h"

namespace gen::detail {
	class AddressWriter {
	private:
		detail::ProcessWriter pProcess;
		detail::MappingWriter pMapping;
		detail::Addresses& pHost;
		wasm::Sink& pSink;
		mutable wasm::Variable pTempAddress;

	public:
		AddressWriter(const detail::ProcessState& process, const detail::MappingState& mapping, detail::Addresses& host, wasm::Sink& sink);

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
