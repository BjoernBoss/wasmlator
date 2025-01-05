#include "../generate.h"

gen::detail::AddressWriter::AddressWriter(const detail::MappingState& mapping, detail::Addresses& host) : pMapping{ mapping }, pHost{ host } {}

void gen::detail::AddressWriter::fCallLandingPad(env::guest_t nextAddress) const {
	if (!pTempAddress.valid())
		pTempAddress = gen::Sink->local(wasm::Type::i64, u8"_continuation_address");

	/* validate the addresses match */
	gen::Add[I::Local::Tee(pTempAddress)];
	gen::Add[I::U64::Const(nextAddress)];
	gen::Add[I::U64::NotEqual()];
	{
		/* return the target-address until someone can execute it
		*	(currently unknown whether or not it has been translated yet) */
		wasm::IfThen _if{ gen::Sink };
		gen::Add[I::Local::Get(pTempAddress)];
		gen::Add[I::Return()];
	}
}

void gen::detail::AddressWriter::makeCall(env::guest_t address, env::guest_t nextAddress) const {
	/* check if the execution is in single-step mode, and simply add a step to the next address */
	if (gen::Instance()->singleStep()) {
		gen::Add[I::U64::Const(address)];
		gen::Add[I::Return()];
		return;
	}
	detail::PlaceAddress target = pHost.pushLocal(address);

	/* check if an immediate call can be added */
	if (target.thisModule)
		gen::Add[I::Call::Direct(target.function)];

	/* add the lookup via the table */
	else {
		/* check if the slot-value needs to be verified and potentially be written back */
		if (!target.alreadyExists) {
			gen::Add[I::U32::Const(target.index)];
			gen::Add[I::Table::Get(pHost.addresses())];
			gen::Add[I::Ref::IsNull()];

			/* perform the lookup and write the function back to the table */
			{
				wasm::IfThen _if{ gen::Sink };

				/* index for the address to be written to */
				gen::Add[I::U32::Const(target.index)];

				/* lookup the actual function corresponding to the address */
				gen::Add[I::U64::Const(address)];
				pMapping.makeGetFunction();

				/* write the address to the table */
				gen::Add[I::Table::Set(pHost.addresses())];
			}
		}

		/* perform the call */
		gen::Add[I::U32::Const(target.index)];
		gen::Add[I::Call::Indirect(pHost.addresses(), pMapping.blockPrototype())];
	}

	/* add the final landing-pad of the return (to validate the return-address) */
	fCallLandingPad(nextAddress);
}
void gen::detail::AddressWriter::makeCallIndirect(env::guest_t nextAddress) const {
	/* check if the execution is in single-step mode, and simply add a step to the next address */
	if (gen::Instance()->singleStep()) {
		gen::Add[I::Return()];
		return;
	}

	/* add the normal indirect call and corresponding landing-pad (to validate the return-address)  */
	pMapping.makeDirectInvoke();
	fCallLandingPad(nextAddress);
}
void gen::detail::AddressWriter::makeJump(env::guest_t address) const {
	/* check if the execution is in single-step mode, and simply add a step to the next address */
	if (gen::Instance()->singleStep()) {
		gen::Add[I::U64::Const(address)];
		gen::Add[I::Return()];
		return;
	}
	detail::PlaceAddress target = pHost.pushLocal(address);

	/* check if an immediate tail-call can be added */
	if (target.thisModule) {
		gen::Add[I::Call::Tail(target.function)];
		return;
	}

	/* check if the slot-value needs to be verified and potentially be written back */
	if (!target.alreadyExists) {
		gen::Add[I::U32::Const(target.index)];
		gen::Add[I::Table::Get(pHost.addresses())];
		gen::Add[I::Ref::IsNull()];

		/* perform the lookup and write the function back to the table */
		{
			wasm::IfThen _if{ gen::Sink };

			/* index for the address to be written to */
			gen::Add[I::U32::Const(target.index)];

			/* lookup the actual function corresponding to the address */
			gen::Add[I::U64::Const(address)];
			pMapping.makeGetFunction();

			/* write the address to the table */
			gen::Add[I::Table::Set(pHost.addresses())];
		}
	}

	/* perform the 'jump' to the address */
	gen::Add[I::U32::Const(target.index)];
	gen::Add[I::Call::IndirectTail(pHost.addresses(), pMapping.blockPrototype())];
}
void gen::detail::AddressWriter::makeJumpIndirect() const {
	/* check if the execution is in single-step mode, and simply add a step to the next address */
	if (gen::Instance()->singleStep()) {
		gen::Add[I::Return()];
		return;
	}

	/* add the indirect jump to the target */
	pMapping.makeTailInvoke();
}
void gen::detail::AddressWriter::makeReturn() const {
	/* check if the execution is in single-step mode, and simply add a step to the next address */
	if (gen::Instance()->singleStep()) {
		gen::Add[I::Return()];
		return;
	}

	/* add the direct return, which will ensure a validation of the target address */
	gen::Add[I::Return()];
}
