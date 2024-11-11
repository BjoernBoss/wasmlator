#include "address-writer.h"

namespace I = wasm::inst;

gen::detail::AddressWriter::AddressWriter(const detail::MappingState& mapping, detail::Addresses& host, wasm::Sink& sink) : pMapping{ mapping, sink }, pHost{ host }, pSink{ sink } {}

void gen::detail::AddressWriter::fCallLandingPad(env::guest_t nextAddress) const {
	if (!pTempRetState.valid())
		pTempRetState = pSink.local(wasm::Type::i32, u8"address_temp_state");
	if (!pTempRetAddress.valid())
		pTempRetAddress = pSink.local(wasm::Type::i64, u8"address_temp_address");


	/* check for the mode not matching */
	pSink[I::Local::Tee(pTempRetState)];
	pSink[I::U32::Const(env::ExecState::_execute)];
	pSink[I::U32::NotEqual()];
	{
		/* simply fully return the paramter to the caller */
		wasm::IfThen _if{ pSink };
		pSink[I::Local::Get(pTempRetState)];
		pSink[I::Return()];
	}

	/* validate the addresses match */
	pSink[I::Local::Tee(pTempRetAddress)];
	pSink[I::U32::Const(nextAddress)];
	pSink[I::U32::NotEqual()];
	{
		/* trigger an execution of the target address */
		wasm::IfThen _if{ pSink };
		pSink[I::Local::Get(pTempRetAddress)];
		pSink[I::U32::Const(env::ExecState::_execute)];
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

			/* ensure that the local variables exist */
			if (!pTempFuncIndex.valid())
				pTempFuncIndex = pSink.local(wasm::Type::i32, u8"address_temp_function");

			/* perform the lookup and write the function back to the table */
			{
				wasm::IfThen _if{ pSink };
				pSink[I::U64::Const(address)];
				pMapping.makeLookup();
				pSink[I::Local::Tee(pTempFuncIndex)];

				/* check if the lookup failed, because the address has not yet been translated */
				pMapping.makeCheckFailed();
				{
					wasm::IfThen _if{ pSink };
					pSink[I::U64::Const(0)];
					pSink[I::U32::Const(env::ExecState::translate)];
					pSink[I::Return()];
				}

				/* write the lookup-result to the linked table */
				pSink[I::U32::Const(target.index)];
				pSink[I::Local::Get(pTempFuncIndex)];
				pMapping.makeLoadFunction();
				pSink[I::Table::Set(pHost.addresses())];
			}
		}

		/* perform the call */
		pSink[I::U32::Const(target.index)];
		pSink[I::Call::Indirect(pHost.addresses(), pHost.blockPrototype())];
	}

	fCallLandingPad(nextAddress);
}
void gen::detail::AddressWriter::makeCallIndirect(env::guest_t nextAddress) const {
	/* ensure that the local variables exist */
	if (!pTempFuncIndex.valid())
		pTempFuncIndex = pSink.local(wasm::Type::i32, u8"address_temp_function");

	/* perform the lookup and write the function back to the table */
	{
		/* lookup the function */
		pMapping.makeLookup();
		pSink[I::Local::Tee(pTempFuncIndex)];

		/* check if the lookup failed, because the address has not yet been translated */
		pMapping.makeCheckFailed();
		{
			wasm::IfThen _if{ pSink };
			pSink[I::U64::Const(0)];
			pSink[I::U32::Const(env::ExecState::translate)];
			pSink[I::Return()];
		}

		/* invoke the function */
		pSink[I::Local::Get(pTempFuncIndex)];
		pMapping.makeInvoke();
	}

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

		/* ensure that the local variables exist */
		if (!pTempFuncIndex.valid())
			pTempFuncIndex = pSink.local(wasm::Type::i32, u8"address_temp_function");

		/* perform the lookup and write the function back to the table */
		{
			wasm::IfThen _if{ pSink };
			pSink[I::U64::Const(address)];
			pMapping.makeLookup();
			pSink[I::Local::Tee(pTempFuncIndex)];

			/* check if the lookup failed, because the address has not yet been translated */
			pMapping.makeCheckFailed();
			{
				wasm::IfThen _if{ pSink };
				pSink[I::U64::Const(0)];
				pSink[I::U32::Const(env::ExecState::translate)];
				pSink[I::Return()];
			}

			/* write the lookup-result to the linked table */
			pSink[I::U32::Const(target.index)];
			pSink[I::Local::Get(pTempFuncIndex)];
			pMapping.makeLoadFunction();
			pSink[I::Table::Set(pHost.addresses())];
		}
	}

	/* perform the 'jump' to the address */
	pSink[I::U32::Const(target.index)];
	pSink[I::Call::IndirectTail(pHost.addresses(), pHost.blockPrototype())];
}
void gen::detail::AddressWriter::makeJumpIndirect() const {
	/* ensure that the local variables exist */
	if (!pTempFuncIndex.valid())
		pTempFuncIndex = pSink.local(wasm::Type::i32, u8"address_temp_function");

	/* perform the lookup and write the function back to the table */
	{
		/* lookup the function */
		pMapping.makeLookup();
		pSink[I::Local::Tee(pTempFuncIndex)];

		/* check if the lookup failed, because the address has not yet been translated */
		pMapping.makeCheckFailed();
		{
			wasm::IfThen _if{ pSink };
			pSink[I::U64::Const(0)];
			pSink[I::U32::Const(env::ExecState::translate)];
			pSink[I::Return()];
		}

		/* invoke the function */
		pSink[I::Local::Get(pTempFuncIndex)];
		pMapping.makeTailInvoke();
	}
}
void gen::detail::AddressWriter::makeReturn() const {
	/* write the execute-state onto the stack and simply return fully */
	pSink[I::U32::Const(env::ExecState::_execute)];
	pSink[I::Return()];
}
