#include "../system.h"

static util::Logger logger{ u8"sys::debugger" };

template <class Type>
static void PrintLine(std::u8string& to, const uint8_t* data, size_t count, const char32_t* fmt, size_t lineBytes) {
	/* write the values themselves out */
	size_t i = 0;
	for (; i < count; i += sizeof(Type)) {
		/* add the spacing */
		if (i > 0)
			to.push_back(u8' ');

		/* add the normal value, should it entirely fit */
		if (i + sizeof(Type) <= count) {
			str::CallFormat(to, *reinterpret_cast<const Type*>(data + i), fmt);
			continue;
		}

		/* write the cut value out */
		size_t rest = count - i;
		for (size_t j = rest; j < sizeof(Type); ++j)
			to.append(u8"  ");
		for (size_t j = rest; j > 0; --j)
			str::CallFormat(to, data[i + j - 1], U"02x");
	}

	/* write the remaining null-entries out */
	for (; i < lineBytes; i += sizeof(Type))
		to.append(1 + 2 * sizeof(Type), u8' ');

	/* write the separator out */
	to.append(u8" | ");

	/* write the chars out */
	for (i = 0; i < count; ++i)
		to.push_back(cp::prop::IsAscii(data[i]) && !cp::prop::IsControl(data[i]) ? char8_t(data[i]) : u8'.');
}

bool sys::Debugger::fSetup(sys::Userspace* userspace) {
	pUserspace = userspace;
	pRegisters = pUserspace->cpu()->debugQueryNames();
	pMode = Mode::none;
	return true;
}
void sys::Debugger::fCheck(env::guest_t address) {
	if (pMode == Mode::disabled)
		return;
	bool _skip = pBreakSkip;
	pBreakSkip = false;

	/* check if a breakpoint has been hit */
	if (pBreakPoints.contains(address) && !_skip) {
		fHalted(address);
		return;
	}

	/* check if the current mode is considered done */
	switch (pMode) {
	case Mode::step:
		if (pCount-- > 0)
			return;
		break;
	case Mode::until:
		if (address != pUntil || _skip)
			return;
		break;
	case Mode::run:
		return;
	default:
		break;
	}

	/* mark the debugger as halted */
	fHalted(address);
}

std::vector<uint8_t> sys::Debugger::fReadBytes(env::guest_t address, size_t bytes) const {
	std::vector<uint8_t> out(bytes);

	/* try to read the data in bulk */
	try {
		env::Instance()->memory().mread(out.data(), address, out.size(), env::Usage::None);
		return out;
	}
	catch (const env::MemoryFault&) {}

	/* read as many bytes as possible */
	out.clear();
	try {
		for (size_t i = 0; i < bytes; ++i) {
			uint8_t bt = env::Instance()->memory().read<uint8_t>(address + i);
			out.push_back(bt);
		}
	}
	catch (const env::MemoryFault&) {}
	return out;
}
void sys::Debugger::fPrintData(env::guest_t address, size_t bytes, uint8_t width) const {
	std::vector<uint8_t> data = fReadBytes(address, bytes);

	/* construct the value-formatter to be used and the format printer itself */
	str::u32::Local<16> fmt = str::lu32<16>::Build(U'0', width * 2, U'x');
	void(*fmtLine)(std::u8string&, const uint8_t*, size_t, const char32_t*, size_t) = 0;
	switch (width) {
	case 2:
		fmtLine = &PrintLine<uint16_t>;
		break;
	case 4:
		fmtLine = &PrintLine<uint32_t>;
		break;
	case 8:
		fmtLine = &PrintLine<uint64_t>;
		break;
	case 1:
	default:
		fmtLine = &PrintLine<uint8_t>;
		break;
	}

	/* write the data out */
	for (size_t i = 0; i < data.size(); i += 16) {
		size_t count = std::min<size_t>(data.size() - i, 16);

		/* construct the line itself and write it out */
		std::u8string line = str::u8::Build(str::As{ U"#018x", address + i }, u8": ");
		fmtLine(line, data.data() + i, count, fmt.c_str(), 16);
		util::nullLogger.log(line);
	}

	/* check if a memory fault occurred */
	if (data.size() < bytes)
		logger.fmtError(u8"[{:#018x}]: Unable to read memory", address + data.size());
}
void sys::Debugger::fHalted(env::guest_t address) {
	/* reset the debugger */
	pMode = Mode::none;
	pCount = 0;
	pUserspace->setPC(address);

	/* print common halting information */
	fPrintCommon();

	throw detail::DebuggerHalt{};
}
void sys::Debugger::fPrintCommon() const {
	printState();
	printInstructions(std::nullopt, 10);
}

void sys::Debugger::run() {
	pMode = Mode::run;
	pBreakSkip = true;
	pUserspace->execute();
}
void sys::Debugger::step(size_t count) {
	pMode = Mode::step;
	pBreakSkip = true;
	if ((pCount = count) > 0)
		pUserspace->execute();
}
void sys::Debugger::until(env::guest_t address) {
	pUntil = address;
	pMode = Mode::until;
	pBreakSkip = true;
	pUserspace->execute();
}
void sys::Debugger::addBreak(env::guest_t address) {
	pBreakPoints.insert(address);
}
void sys::Debugger::dropBreak(env::guest_t address) {
	pBreakPoints.erase(address);
}

void sys::Debugger::printState() const {
	sys::Cpu* cpu = pUserspace->cpu();

	/* print all registers */
	for (size_t i = 0; i < pRegisters.size(); ++i) {
		uint64_t value = cpu->debugGetValue(i);
		util::nullLogger.log(pRegisters[i], u8": ", str::As{ U"#018x", value }, u8" (", value, u8')');
	}

	/* print the pc */
	env::guest_t addr = pUserspace->getPC();
	util::nullLogger.log(u8"pc: ", str::As{ U"#018x", addr }, u8" (", addr, u8')');
}
void sys::Debugger::printBreaks() const {
	for (env::guest_t addr : pBreakPoints)
		util::nullLogger.log(str::As{ U"#018x", addr });
}
void sys::Debugger::printInstructions(std::optional<env::guest_t> address, size_t count) const {
	env::guest_t pc = pUserspace->getPC();
	env::guest_t addr = address.value_or(pc);
	sys::Cpu* cpu = pUserspace->cpu();

	try {
		while (count-- > 0) {
			/* decode the next instruction and check if the decoding failed */
			auto [str, size] = cpu->debugDecode(addr);
			if (size == 0) {
				util::nullLogger.fmtLog(u8"[{:#018x}]: Unable to decode", addr);
				break;
			}

			/* print the instruction and advance the address */
			util::nullLogger.fmtLog(u8"  {} [{:#018x}]: {}", (addr == pc ? u8'>' : u8' '), addr, str);
			addr += size;
		}
	}

	/* check if a memory fault occurred */
	catch (const env::MemoryFault&) {
		logger.fmtError(u8"[{:#018x}]: Unable to read memory", addr);
	}
}
void sys::Debugger::printData8(env::guest_t address, size_t bytes) const {
	fPrintData(address, bytes, 1);
}
void sys::Debugger::printData16(env::guest_t address, size_t bytes) const {
	fPrintData(address, bytes, 2);
}
void sys::Debugger::printData32(env::guest_t address, size_t bytes) const {
	fPrintData(address, bytes, 4);
}
void sys::Debugger::printData64(env::guest_t address, size_t bytes) const {
	fPrintData(address, bytes, 8);
}
