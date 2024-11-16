#include "address-writer.h"

namespace I = wasm::inst;

gen::detail::AddressWriter::AddressWriter(const detail::MappingState& mapping, detail::Addresses& host, wasm::Sink& sink) : pMapping{ mapping, sink }, pHost{ host }, pSink{ sink } {}

void gen::detail::AddressWriter::fCallLandingPad(env::guest_t nextAddress) const {
	if (!pTempAddress.valid())
		pTempAddress = pSink.local(wasm::Type::i64, u8"temp_continuation_address");

	/* validate the addresses match */
	pSink[I::Local::Tee(pTempAddress)];
	pSink[I::U64::Const(nextAddress)];
	pSink[I::U64::NotEqual()];
	{
		/* return the target-address until someone can execute it
		*	(currently unknown whether or not it has been translated yet) */
		wasm::IfThen _if{ pSink };
		pSink[I::Local::Get(pTempAddress)];
		pSink[I::Return()];
	}
}

void gen::detail::AddressWriter::makeCall(env::guest_t address, env::guest_t nextAddress) const {
	detail::PlaceAddress target = pHost.pushLocal(address);

	/* check if an immediate call can be added */
	if (target.thisModule)
		pSink[I::Call::Direct(target.function)];

	/* add the lookup via the table */
	else {
		/* check if the slot-value needs to be verified and potentially be written back */
		if (!target.alreadyExists) {
			pSink[I::U32::Const(target.index)];
			pSink[I::Table::Get(pHost.addresses())];
			pSink[I::Ref::IsNull()];

			/* perform the lookup and write the function back to the table */
			{
				wasm::IfThen _if{ pSink };

				/* index for the address to be written to */
				pSink[I::U32::Const(target.index)];

				/* lookup the actual function corresponding to the address */
				pSink[I::U64::Const(address)];
				pMapping.makeGetFunction();

				/* write the address to the table */
				pSink[I::Table::Set(pHost.addresses())];
			}
		}

		/* perform the call */
		pSink[I::U32::Const(target.index)];
		pSink[I::Call::Indirect(pHost.addresses(), pMapping.blockPrototype())];
	}

	fCallLandingPad(nextAddress);
}
void gen::detail::AddressWriter::makeCallIndirect(env::guest_t nextAddress) const {
	pMapping.makeDirectInvoke();
	fCallLandingPad(nextAddress);
}
void gen::detail::AddressWriter::makeJump(env::guest_t address) const {
	detail::PlaceAddress target = pHost.pushLocal(address);

	/* check if an immediate tail-call can be added */
	if (target.thisModule) {
		pSink[I::Call::Tail(target.function)];
		return;
	}

	/* check if the slot-value needs to be verified and potentially be written back */
	if (!target.alreadyExists) {
		pSink[I::U32::Const(target.index)];
		pSink[I::Table::Get(pHost.addresses())];
		pSink[I::Ref::IsNull()];

		/* perform the lookup and write the function back to the table */
		{
			wasm::IfThen _if{ pSink };

			/* index for the address to be written to */
			pSink[I::U32::Const(target.index)];

			/* lookup the actual function corresponding to the address */
			pSink[I::U64::Const(address)];
			pMapping.makeGetFunction();

			/* write the address to the table */
			pSink[I::Table::Set(pHost.addresses())];
		}
	}

	/* perform the 'jump' to the address */
	pSink[I::U32::Const(target.index)];
	pSink[I::Call::IndirectTail(pHost.addresses(), pMapping.blockPrototype())];
}
void gen::detail::AddressWriter::makeJumpIndirect() const {
	pMapping.makeDirectInvoke();
}
void gen::detail::AddressWriter::makeReturn() const {
	pSink[I::Return()];
}
