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
void rv64::detail::TranslateJAL(const gen::Instruction& gen, const rv64::Instruction& inst, const gen::Writer& writer, const wasm::Variable& tempI64) {
	wasm::Sink& sink = writer.sink();

	/* write the next pc to the destination register */
	if (inst.dest != reg::Zero) {
		LoadRegister(reg::PC, writer);
		sink[I::U64::Const(4)];
		sink[I::U64::Add()];
		StoreRegister(inst.dest, writer);
	}

	/* perform the operations on the pc */
	if (inst.opcode == rv64::Opcode::jump_and_link_imm)
		LoadRegister(reg::PC, writer);
	else
		LoadRegister(inst.src1, writer);
	sink[I::I64::Const(inst.imm)];
	sink[I::I64::Add()];
	sink[I::Local::Tee(tempI64)];
	StoreRegister(reg::PC, writer);

	/* check if the indirect jump can be predicted somehow (based on the riscv specification) */
	bool rdIsLink = (inst.dest == reg::X1 || inst.dest == reg::X5);
	if (inst.opcode == rv64::Opcode::jump_and_link_reg) {
		bool rs1IsLink = (inst.src1 == reg::X1 || inst.src1 == reg::X5);

		/* check if the instruction can be considered a return */
		if (!rdIsLink && rs1IsLink) {
			sink[I::Local::Get(tempI64)];
			writer.ret();
		}

		/* check if the instruction can be considered a call */
		else if (rdIsLink && (!rs1IsLink || inst.dest == inst.src1)) {
			sink[I::Local::Get(tempI64)];
			writer.call(gen);
		}

		/* translate the instruction as normal jump */
		else {
			sink[I::Local::Get(tempI64)];
			writer.jump();
		}
		return;
	}

	/* add the instruction as normal direct call or jump (assumption that jump with rd = link implies call) */
	if (rdIsLink)
		writer.call(gen.address + inst.imm, gen);
	else if (const wasm::Target* target = writer.hasTarget(gen); target != 0)
		sink[I::Branch::Direct(*target)];
	else
		writer.jump(gen.address + inst.imm);
}

void rv64::Translate(const gen::Instruction& gen, const rv64::Instruction& inst, const gen::Writer& writer, const wasm::Variable& tempI64) {
	wasm::Sink& sink = writer.sink();

	switch (inst.opcode) {
	case rv64::Opcode::load_upper_imm:
		if (inst.dest != reg::Zero) {
			sink[I::U64::Const(inst.imm)];
			detail::StoreRegister(inst.dest, writer);
		}
		break;
	case rv64::Opcode::add_imm:
		if (inst.dest != reg::Zero) {
			detail::LoadRegister(inst.src1, writer);
			sink[I::I64::Const(inst.imm)];
			sink[I::I64::Add()];
			detail::StoreRegister(inst.dest, writer);
		}
		break;
	case rv64::Opcode::jump_and_link_imm:
	case rv64::Opcode::jump_and_link_reg:
		detail::TranslateJAL(gen, inst, writer, tempI64);
		break;
	default:
		host::Fatal(u8"Instruction [", size_t(inst.opcode), u8"] currently not implemented");
	}
}
