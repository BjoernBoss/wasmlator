#include "rv64-cpu.h"

static util::Logger logger{ u8"rv64::cpu" };

rv64::Cpu::Cpu() : sys::Cpu{ u8"RISC-V 64", rv64::MemoryCaches, sizeof(rv64::Context), false, false, sys::ArchType::riscv64 } {}

rv64::Instruction rv64::Cpu::fFetchRaw(env::guest_t address) const {
	rv64::Instruction inst;

	/* check that the address is 2-byte aligned */
	if ((address & 0x01) != 0) {
		inst.opcode = rv64::Opcode::misaligned;
		inst.size = 1;
		return inst;
	}

	/* decode the next instruction (first as compressed, and afterwards as large, in order to
	*	prevent potential memory-alignment issues - keep in mind that reading the memory may
	*	throw an exception, therefore handle the instruction such that each read could abort) */
	uint32_t code = env::Instance()->memory().code<uint16_t>(address);
	inst = rv64::Decode16(uint16_t(code));
	if (inst.opcode != rv64::Opcode::_invalid)
		return inst;

	/* decode the instruction as 32-bit instruction */
	code |= (env::Instance()->memory().code<uint16_t>(address + 2) << 16);
	inst = rv64::Decode32(code);

	/* check if the instruction is invalid and patch its size to be 0 */
	if (inst.opcode == rv64::Opcode::_invalid)
		inst.size = 0;
	return inst;
}
rv64::Instruction rv64::Cpu::fFetch(env::guest_t address) const {
	/* fetch the initial instruction (let memory-exceptions pass through) */
	rv64::Instruction inst = fFetchRaw(address);

	/* check if it might be a pseudo-instruction */
	rv64::DetectPseudo pseudo{ !gen::Instance()->debugCheck() };
	try {
		while (pseudo.next(inst))
			inst = fFetchRaw(address += inst.size);
	}

	/* catch any memory-exceptions and close the psudo-detector (dont propagate memory-exceptions
	*	through, as they are not technically part of the current instruction) */
	catch (const env::MemoryFault&) {
		inst = pseudo.close();
	}
	return inst;
}

uint64_t rv64::Cpu::fHandleHWProbe(uint64_t pairs, uint64_t pairCount, uint64_t cpuCount, uint64_t cpus, uint64_t flags) const {
	/* Note: cpus and flags are ignored */
	try {
		/* iterate over the pair and write the values out */
		for (size_t i = 0; i < pairCount; i++) {
			switch (env::Instance()->memory().read<uint64_t>(pairs + 16 * i + 0)) {
				/* RISCV_HWPROBE_KEY_MVENDORID: 0 */
			case 0:
				env::Instance()->memory().write<uint64_t>(pairs + 16 * i + 8, 0xdeadbeef);
				break;

				/* RISCV_HWPROBE_KEY_MARCHID: 1*/
			case 1:
				env::Instance()->memory().write<uint64_t>(pairs + 16 * i + 8, 0x5555'5555'5555'5555);
				break;

				/* RISCV_HWPROBE_KEY_MIMPID: 2 */
			case 2:
				env::Instance()->memory().write<uint64_t>(pairs + 16 * i + 8, 0xaaaa'aaaa'aaaa'aaaa);
				break;

				/* RISCV_HWPROBE_KEY_BASE_BEHAVIOR: 3 */
			case 3:
				/* RISCV_HWPROBE_BASE_BEHAVIOR_IMA:1 (to indicate that Integer/Multiplication/Atomic is supported) */
				env::Instance()->memory().write<uint64_t>(pairs + 16 * i + 8, 1);
				break;

				/* RISCV_HWPROBE_KEY_IMA_EXT_0: 4 */
			case 4:
				/* 0:Float/Double | 1:Compressed */
				env::Instance()->memory().write<uint64_t>(pairs + 16 * i + 8, (1 << 0) | (1 << 1));
				break;
			default:
				env::Instance()->memory().write<int64_t>(pairs + 16 * i + 0, -1);
				env::Instance()->memory().write<int64_t>(pairs + 16 * i + 8, 0);
				break;
			}
		}
	}

	/* catch any memory faults and simply return the fault-error */
	catch (const env::MemoryFault&) {
		return sys::errCode::eFault;
	}
	return sys::errCode::eSuccess;
}

std::unique_ptr<sys::Cpu> rv64::Cpu::New() {
	return std::unique_ptr<rv64::Cpu>{ new rv64::Cpu{} };
}

bool rv64::Cpu::setupCpu(sys::Writer* writer) {
	pWriter = writer;
	return true;
}
bool rv64::Cpu::setupCore(wasm::Module& mod) {
	return pTranslator.setup();
}
bool rv64::Cpu::setupContext(env::guest_t pcAddress, env::guest_t spAddress) {
	rv64::Context& ctx = env::Instance()->context().get<rv64::Context>();

	/* no need to write the pc to the context, as all exceptions will implicitly
	*	already know the exact pc but write the stack-pointer to the sp-register,
	*	the rest can remain as-is (will implicitly be null) */
	ctx.sp = spAddress;

	return true;
}

void rv64::Cpu::started(env::guest_t address) {
	pDecoded.clear();
	pTranslator.resetAll(pWriter);
}
void rv64::Cpu::completed() {
	pDecoded.clear();
	pTranslator.resetAll(0);
}
gen::Instruction rv64::Cpu::fetch(env::guest_t address) {
	/* decode the next instruction (keep in mind that fFetch may throw a memory-exception) */
	rv64::Instruction inst = fFetch(address);
	pDecoded.push_back(inst);
	if (env::Instance()->logBlocks())
		logger.fmtTrace(u8"RV64: {:#018x} {}", address, rv64::ToString(inst));

	/* translate the instruction to the generated instruction-type */
	gen::InstType type = gen::InstType::primitive;
	env::guest_t target = 0;
	switch (inst.opcode) {
	case rv64::Opcode::misaligned:
		type = gen::InstType::endOfBlock;
		break;
	case rv64::Opcode::_invalid:
		type = gen::InstType::invalid;
		break;
	case rv64::Opcode::branch_eq:
	case rv64::Opcode::branch_ne:
	case rv64::Opcode::branch_lt_s:
	case rv64::Opcode::branch_ge_s:
	case rv64::Opcode::branch_lt_u:
	case rv64::Opcode::branch_ge_u:
		type = gen::InstType::conditionalDirect;
		target = address + inst.imm;
		break;
	case rv64::Opcode::jump_and_link_imm:
		/* check if it might be a call and otherwise consider it a direct jump */
		if (!inst.isCall()) {
			type = gen::InstType::jumpDirect;
			target = address + inst.imm;
		}
		break;
	case rv64::Opcode::jump_and_link_reg:
		/* check if it might be a call and otherwise consider it a return or indirect jump */
		if (!inst.isCall())
			type = gen::InstType::endOfBlock;
		break;
	default:
		break;
	}

	/* check if the target is considered mis-aligned, in which case it does not need to be translated */
	if (inst.isMisaligned(address))
		type = gen::InstType::endOfBlock;

	/* construct the output-instruction format */
	return gen::Instruction{ type, target, inst.size, pDecoded.size() - 1 };
}
void rv64::Cpu::produce(env::guest_t address, const uintptr_t* self, size_t count) {
	pTranslator.start(address);
	for (size_t i = 0; i < count; ++i)
		pTranslator.next(pDecoded[self[i]]);
}

sys::SyscallArgs rv64::Cpu::syscallGetArgs() const {
	/*
	*	syscall calling convention:
	*	args in [a0, ..., a5]
	*	syscall-index in [a7]
	*	result into a0
	*/
	sys::SyscallArgs call;
	rv64::Context& ctx = env::Instance()->context().get<rv64::Context>();

	/* fetch the arguments */
	for (size_t i = 0; i < 6; ++i)
		call.args[i] = ctx.iregs[reg::A0 + i];
	call.rawIndex = ctx.a7;

	/* check if its the vendor specific syscall 'riscv_hwprobe' */
	if (call.rawIndex == 258) {
		call.args[0] = fHandleHWProbe(call.args[0], call.args[1], call.args[2], call.args[3], call.args[4]);
		call.index = sys::SyscallIndex::completed;
		return call;
	}

	/* map the index */
	switch (call.rawIndex) {
	case 29:
		call.index = sys::SyscallIndex::ioctl;
		break;
	case 48:
		call.index = sys::SyscallIndex::faccessat;
		break;
	case 56:
		call.index = sys::SyscallIndex::openat;
		break;
	case 57:
		call.index = sys::SyscallIndex::close;
		break;
	case 63:
		call.index = sys::SyscallIndex::read;
		break;
	case 64:
		call.index = sys::SyscallIndex::write;
		break;
	case 65:
		call.index = sys::SyscallIndex::readv;
		break;
	case 66:
		call.index = sys::SyscallIndex::writev;
		break;
	case 78:
		call.index = sys::SyscallIndex::readlinkat;
		break;
	case 79:
		call.index = sys::SyscallIndex::fstatat;
		break;
	case 80:
		call.index = sys::SyscallIndex::fstat;
		break;
	case 94:
		call.index = sys::SyscallIndex::exit_group;
		break;
	case 96:
		call.index = sys::SyscallIndex::set_tid_address;
		break;
	case 99:
		call.index = sys::SyscallIndex::set_robust_list;
		break;
	case 113:
		call.index = sys::SyscallIndex::clock_gettime;
		break;
	case 160:
		call.index = sys::SyscallIndex::uname;
		break;
	case 169:
		call.index = sys::SyscallIndex::gettimeofday;
		break;
	case 174:
		call.index = sys::SyscallIndex::getuid;
		break;
	case 175:
		call.index = sys::SyscallIndex::geteuid;
		break;
	case 176:
		call.index = sys::SyscallIndex::getgid;
		break;
	case 177:
		call.index = sys::SyscallIndex::getegid;
		break;
	case 179:
		call.index = sys::SyscallIndex::sysinfo;
		break;
	case 214:
		call.index = sys::SyscallIndex::brk;
		break;
	case 215:
		call.index = sys::SyscallIndex::munmap;
		break;
	case 222:
		call.index = sys::SyscallIndex::mmap;
		break;
	case 226:
		call.index = sys::SyscallIndex::mprotect;
		break;
	case 261:
		call.index = sys::SyscallIndex::prlimit64;
		break;
	case 278:
		call.index = sys::SyscallIndex::getrandom;
		break;
	case 439:
		call.index = sys::SyscallIndex::faccessat2;
		break;
	default:
		call.index = sys::SyscallIndex::unknown;
		break;
	}
	return call;
}
void rv64::Cpu::syscallSetResult(uint64_t value) {
	env::Instance()->context().get<rv64::Context>().a0 = value;
}
std::u8string rv64::Cpu::getExceptionText(uint64_t id) const {
	switch (id) {
	case rv64::Translate::EBreakException:
		return u8"ebreak";
	case rv64::Translate::MisalignedException:
		return u8"misaligned instruction";
	case rv64::Translate::IllegalException:
		return u8"illegal instruction";
	case rv64::Translate::CsrUnsupported:
		return u8"unsupported csr register";
	case rv64::Translate::NotImplException:
		return u8"not-implemented";
	default:
		return u8"%unknown%";
	}
}

std::vector<std::u8string> rv64::Cpu::debugQueryNames() const {
	return {
		u8"ra", u8"sp", u8"gp", u8"tp",
		u8"t0", u8"t1", u8"t2", u8"fp",
		u8"s1", u8"a0", u8"a1", u8"a2",
		u8"a3", u8"a4", u8"a5",u8"a6",
		u8"a7", u8"s2", u8"s3",u8"s4",
		u8"s5", u8"s6", u8"s7", u8"s8",
		u8"s9", u8"s10", u8"s11", u8"t3",
		u8"t4", u8"t5", u8"t6"
	};
}
std::pair<std::u8string, uint8_t> rv64::Cpu::debugDecode(env::guest_t address) const {
	rv64::Instruction inst = fFetch(address);
	return { rv64::ToString(inst), inst.size };
}
uint64_t rv64::Cpu::debugGetValue(size_t index) const {
	return env::Instance()->context().get<rv64::Context>().iregs[index + 1];
}
void rv64::Cpu::debugSetValue(size_t index, uint64_t value) {
	env::Instance()->context().get<rv64::Context>().iregs[index + 1] = value;
}
