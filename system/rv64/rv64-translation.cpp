#include "rv64-translation.h"

namespace I = wasm::inst;

void rv64::detail::LoadRegister(uint8_t reg, const gen::Writer& writer) {
	/* check if its the zero-register and simply load zero, otherwise read the value from the context */
	if (reg == reg::Zero)
		writer.sink()[I::U64::Const(0)];
	else
		writer.ctxRead(reg * sizeof(uint64_t), gen::MemoryType::i64);
}
void rv64::detail::StoreRegister(uint8_t reg, const gen::Writer& writer) {
	/* check if its the zero-register and ignore the operation */
	if (reg != reg::Zero)
		writer.ctxWrite(reg * sizeof(uint64_t), gen::MemoryType::i64);
	else
		writer.sink()[I::Drop()];
}
void rv64::detail::TranslateJAL(const gen::Instruction& gen, const rv64::Instruction& inst, const gen::Writer& writer) {
	wasm::Sink& sink = writer.sink();

	/* write the next pc to the destination register */
	if (inst.dest != reg::Zero) {
		sink[I::U64::Const(gen.address + gen.size)];
		StoreRegister(inst.dest, writer);
	}

	/* check if its an indirect jump and if it can be predicted somehow (based on the riscv specification) */
	if (inst.opcode == rv64::Opcode::jump_and_link_reg) {
		/* write the target pc to the stack */
		sink[I::I64::Const(inst.imm)];
		if (inst.src1 != reg::Zero) {
			LoadRegister(inst.src1, writer);
			sink[I::I64::Add()];
		}

		/* check if the instruction can be considered a return */
		if (inst.isRet())
			writer.ret();

		/* check if the instruction can be considered a call */
		else if (inst.isCall())
			writer.call(gen);

		/* translate the instruction as normal jump */
		else
			writer.jump();
		return;
	}

	/* add the instruction as normal direct call or jump */
	env::guest_t address = (gen.address + inst.imm);
	if (inst.isCall())
		writer.call(address, gen);
	else if (const wasm::Target* target = writer.hasTarget(gen); target != 0)
		sink[I::Branch::Direct(*target)];
	else
		writer.jump(address);
}

void rv64::Translate(const gen::Instruction& gen, const rv64::Instruction& inst, const gen::Writer& writer) {
	wasm::Sink& sink = writer.sink();

	switch (inst.opcode) {
	case rv64::Opcode::load_upper_imm:
		if (inst.dest != reg::Zero) {
			sink[I::U64::Const(inst.imm)];
			detail::StoreRegister(inst.dest, writer);
		}
		break;
	case rv64::Opcode::add_upper_imm_pc:
		if (inst.dest != reg::Zero) {
			sink[I::U64::Const(inst.imm + gen.address + gen.size)];
			detail::StoreRegister(inst.dest, writer);
		}
		break;
	case rv64::Opcode::add_imm:
		if (inst.dest != reg::Zero) {
			sink[I::I64::Const(inst.imm)];
			if (inst.src1 != reg::Zero) {
				detail::LoadRegister(inst.src1, writer);
				sink[I::I64::Add()];
			}
			detail::StoreRegister(inst.dest, writer);
		}
		break;
	case rv64::Opcode::jump_and_link_imm:
	case rv64::Opcode::jump_and_link_reg:
		detail::TranslateJAL(gen, inst, writer);
		break;
	default:
		host::Fatal(u8"Instruction [", size_t(inst.opcode), u8"] currently not implemented");
	}
}
