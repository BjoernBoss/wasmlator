/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
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
	pCpu = pUserspace->cpu();
	pRegisters = pCpu->debugQueryNames();
	pHalt.mode = Mode::none;
	return true;
}
void sys::Debugger::fCheck(env::guest_t address) {
	if (pHalt.mode == Mode::disabled)
		return;
	bool _skip = pHalt.breakSkip;
	pHalt.breakSkip = false;

	/* check if a breakpoint has been hit */
	if (pBreakPoints.contains(address) && !_skip) {
		fHalted(address);
		return;
	}

	/* check if the current mode is considered done */
	switch (pHalt.mode) {
	case Mode::step:
		if (pHalt.count-- > 0)
			return;
		break;
	case Mode::until:
		if (address != pHalt.until || _skip)
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
void sys::Debugger::fHalted(env::guest_t address) {
	/* reset the debugger */
	pHalt.mode = Mode::none;
	pUserspace->setPC(address);

	/* print bindings for having halten */
	fPrintBindings();
	throw detail::DebuggerHalt{};
}

void sys::Debugger::fAddBinding(const PrintBinding& binding) {
	pBindings.emplace_back(binding).index = pNextBound++;
	util::nullLogger.log(u8"Binding ", (pNextBound - 1), u8" added");
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
bool sys::Debugger::fParseExpression(std::u8string_view exp, Expression& out) const {
	/* parse all values of the form [+-]? (num | reg) ([+-] (num | reg))* */
	bool lastWasSign = false, subtract = false;
	for (size_t i = 0; i < exp.size(); ++i) {
		/* skip any whitespace */
		if (!cp::prop::IsAscii(exp[i])) {
			logger.error(u8"Invalid token encountered in expression");
			return false;
		}
		if (cp::prop::IsControl(exp[i]) || exp[i] == u8' ')
			continue;

		/* check if a subtraction or addition is to be performed */
		if (!lastWasSign) {
			lastWasSign = true;
			if (exp[i] == u8'+' || exp[i] == u8'-') {
				if (out.empty())
					out.push_back({ 0, 0, false, false, false });
				subtract = (exp[i] == u8'-');
				continue;
			}

			/* check if a sign was expected */
			if (!out.empty()) {
				logger.error(u8"Expected [+] or [-]");
				return false;
			}

			/* this is the first implicit '+' */
		}
		lastWasSign = false;

		/* check if its a number (if token starts either as decimal or with a leading 0 for the prefix) */
		if (cp::ascii::GetRadix(exp[i]) < 10) {
			auto [value, consumed, result] = str::ParseNum<uint64_t>(exp.substr(i), { .radix = 10, .prefix = str::PrefixMode::overwrite });
			if (result != str::NumResult::valid) {
				logger.error(u8"Malformed number encountered");
				return false;
			}
			i += (consumed - 1);
			out.push_back({ 0, value, false, false, subtract });
			continue;
		}

		/* check if its the pc */
		if (exp.substr(i).starts_with(u8"pc")) {
			++i;
			out.push_back({ 0, 0, false, true, subtract });
			continue;
		}

		/* match the register */
		for (size_t j = 0;; ++j) {
			if (j == pRegisters.size()) {
				logger.error(u8"Unknown register encountered");
				return false;
			}
			if (!exp.substr(i).starts_with(pRegisters[j]))
				continue;
			i += pRegisters[j].size() - 1;
			out.push_back({ j, 0, true, false, subtract });
			break;
		}
	}

	/* check if a valid expression was found */
	if (out.empty())
		logger.error(u8"Expression cannot be empty");
	if (lastWasSign)
		logger.error(u8"Expression cannot end on a sign");
	return !out.empty();
}
uint64_t sys::Debugger::fEvalExpression(const Expression& ops) const {
	uint64_t out = 0;

	/* iterate over the values and either subtract or add them to the final output */
	for (size_t i = 0; i < ops.size(); ++i) {
		uint64_t val = 0;

		/* extract the value to be used */
		if (ops[i].usePC)
			val = pUserspace->getPC();
		else if (ops[i].useRegister)
			val = pCpu->debugGetValue(ops[i].regIndex);
		else
			val = ops[i].immediate;

		/* fold it to the result */
		if (ops[i].subtract)
			out -= val;
		else
			out += val;
	}
	return out;
}
std::optional<uint64_t> sys::Debugger::fParseAndEval(std::u8string_view exp) const {
	Expression ops;
	if (!fParseExpression(exp, ops))
		return std::nullopt;
	return fEvalExpression(ops);
}

void sys::Debugger::fPrintInstructions(const Expression& address, size_t count) const {
	/* evaluate the address to be used */
	uint64_t _address = fEvalExpression(address);
	uint64_t pc = pUserspace->getPC();

	try {
		while (count-- > 0) {
			/* decode the next instruction and check if the decoding failed */
			auto [str, size] = pCpu->debugDecode(_address);
			if (size == 0) {
				util::nullLogger.fmtError(u8"[{:#018x}]: Unable to decode", _address);
				break;
			}

			/* print the instruction and advance the address */
			util::nullLogger.fmtLog(u8"  {} [{:#018x}]: {}", (_address == pc ? u8'>' : u8' '), _address, str);
			_address += size;
		}
	}

	/* check if a memory fault occurred */
	catch (const env::MemoryFault&) {
		logger.fmtError(u8"[{:#018x}]: Unable to read memory", _address);
	}
}
void sys::Debugger::fPrintData(const Expression& address, size_t bytes, uint8_t width) const {
	uint64_t _address = fEvalExpression(address);
	std::vector<uint8_t> data = fReadBytes(_address, bytes);

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
		std::u8string line = str::u8::Build(str::As{ U"#018x", _address + i }, u8": ");
		fmtLine(line, data.data() + i, count, fmt.c_str(), 16);
		util::nullLogger.log(line);
	}

	/* check if a memory fault occurred */
	if (data.size() < bytes)
		logger.fmtError(u8"[{:#018x}]: Unable to read memory", _address + data.size());
}
void sys::Debugger::fPrintEval(const Expression& value, const std::u8string& msg, bool hex) const {
	util::nullLogger.log(str::u8::Build(msg, str::As{ (hex ? U"#018x" : U""), fEvalExpression(value) }));
}
void sys::Debugger::fPrintState() const {
	std::u8string_view whitespace = u8"    ";

	/* print all registers */
	for (size_t i = 0; i < pRegisters.size(); ++i) {
		uint64_t value = pCpu->debugGetValue(i);
		std::u8string_view space = whitespace.substr(0, whitespace.size() - std::min<size_t>(whitespace.size(), pRegisters[i].size()));
		util::nullLogger.log(space, pRegisters[i], u8": ", str::As{ U"#018x", value }, u8" (", value, u8')');
	}

	/* print the pc */
	env::guest_t addr = pUserspace->getPC();
	util::nullLogger.log(whitespace.substr(whitespace.size() - 2), u8"pc: ", str::As{ U"#018x", addr }, u8" (", addr, u8')');
}
void sys::Debugger::fPrintBindings() const {
	/* iterate over the bindings and print them */
	for (size_t i = 0; i < pBindings.size(); ++i) {
		switch (pBindings[i].type) {
		case BindType::echo:
			util::nullLogger.log(pBindings[i].msg);
			break;
		case BindType::state:
			fPrintState();
			break;
		case BindType::evalDec:
			fPrintEval(pBindings[i].expression, pBindings[i].msg, false);
			break;
		case BindType::evalHex:
			fPrintEval(pBindings[i].expression, pBindings[i].msg, true);
			break;
		case BindType::inst:
			fPrintInstructions(pBindings[i].expression, pBindings[i].misc);
			break;
		case BindType::data8:
			fPrintData(pBindings[i].expression, pBindings[i].misc, 1);
			break;
		case BindType::data16:
			fPrintData(pBindings[i].expression, pBindings[i].misc, 2);
			break;
		case BindType::data32:
			fPrintData(pBindings[i].expression, pBindings[i].misc, 4);
			break;
		case BindType::data64:
			fPrintData(pBindings[i].expression, pBindings[i].misc, 8);
			break;
		default:
			break;
		}
	}
}

void sys::Debugger::run() {
	pHalt.mode = Mode::run;
	pHalt.breakSkip = true;
	pUserspace->execute();
}
void sys::Debugger::step(size_t count) {
	pHalt.mode = Mode::step;
	pHalt.breakSkip = true;
	if ((pHalt.count = count) > 0)
		pUserspace->execute();
}
void sys::Debugger::until(const std::u8string& address) {
	/* parse the address */
	std::optional<uint64_t> _address = fParseAndEval(address);
	if (!_address.has_value())
		return;

	/* setup the operation and start the execution */
	pHalt.until = _address.value();
	pHalt.mode = Mode::until;
	pHalt.breakSkip = true;
	pUserspace->execute();
}
void sys::Debugger::addBreak(const std::u8string& address) {
	/* parse the address */
	std::optional<uint64_t> _address = fParseAndEval(address);
	if (!_address.has_value())
		return;

	/* define the new breakpoint */
	pBreakPoints.insert(_address.value());
	pBreakIndices[pNextBreakPoint++] = _address.value();
	util::nullLogger.log(u8"Breakpoint ", (pNextBreakPoint - 1), u8" added at ", str::As{ U"#018x", _address.value() });
}
void sys::Debugger::dropBreak(size_t index) {
	auto it = pBreakIndices.find(index);
	if (it == pBreakIndices.end())
		return;
	pBreakPoints.erase(it->second);
	pBreakIndices.erase(it);
}
void sys::Debugger::dropBinding(size_t index) {
	for (size_t i = 0; i < pBindings.size(); ++i) {
		if (pBindings[i].index != index)
			continue;
		pBindings.erase(pBindings.begin() + i);
		break;
	}
}
void sys::Debugger::setupCommon(std::optional<std::u8string> spName) {
	pBindings.clear();
	fAddBinding({ {}, u8"----------------------------------------------------------------", 0, 0, BindType::echo });
	if (spName.has_value()) {
		const std::u8string& sp = spName.value();
		for (size_t i = 0; i < 5; ++i) {
			Expression exp;
			if (!fParseExpression(str::lu8<32>::Build(sp, u8" + ", (4 - i) * 16), exp))
				return;
			fAddBinding({ exp, u8"", 0, 16, BindType::data64 });
		}
		fAddBinding({ {}, u8"", 0, 0, BindType::echo });
	}
	fAddBinding({ {}, u8"", 0, 0, BindType::state });
	fAddBinding({ {}, u8"", 0, 0, BindType::echo });
	fAddBinding({ { Operation{ 0, 0, false, true, false } }, u8"", 0, 10, BindType::inst });
	fAddBinding({ {}, u8"----------------------------------------------------------------", 0, 0, BindType::echo });
}

void sys::Debugger::printBreaks() const {
	for (const auto& [index, address] : pBreakIndices)
		util::nullLogger.log(index, u8": ", str::As{ U"#018x", address });
}
void sys::Debugger::printBindings() const {
	for (const auto& [exp, msg, index, count, type] : pBindings) {
		/* convert the expression to a string */
		std::u8string _exp;
		for (size_t i = 0; i < exp.size(); ++i) {
			if (i > 0)
				_exp.append(exp[i].subtract ? u8" - " : u8" + ");
			else if (exp[i].subtract)
				_exp.push_back(u8'-');

			if (exp[i].usePC)
				_exp.append(u8"pc");
			else if (exp[i].useRegister)
				_exp.append(pRegisters[exp[i].regIndex]);
			else
				str::IntTo(_exp, exp[i].immediate, { .radix = 16, .style = str::NumStyle::lowerWithPrefix });
		}

		/* print the description */
		switch (type) {
		case BindType::echo:
			util::nullLogger.log(index, u8": echo [", msg, u8']');
			break;
		case BindType::state:
			util::nullLogger.log(index, u8": state");
			break;
		case BindType::evalDec:
			util::nullLogger.log(index, u8": eval [", _exp, u8"] as dec and msg [", msg, u8']');
			break;
		case BindType::evalHex:
			util::nullLogger.log(index, u8": eval [", _exp, u8"] as hex and msg [", msg, u8']');
			break;
		case BindType::inst:
			util::nullLogger.log(index, u8": [", count, u8"] instructions at [", _exp, u8']');
			break;
		case BindType::data8:
			util::nullLogger.log(index, u8": [", count, u8"] bytes at [", _exp, u8']');
			break;
		case BindType::data16:
			util::nullLogger.log(index, u8": [", count, u8"] words at [", _exp, u8']');
			break;
		case BindType::data32:
			util::nullLogger.log(index, u8": [", count, u8"] dwords at [", _exp, u8']');
			break;
		case BindType::data64:
			util::nullLogger.log(index, u8": [", count, u8"] qwords at [", _exp, u8']');
			break;
		default:
			break;
		}
	}
}
void sys::Debugger::printEcho(const std::u8string& msg, bool bind) {
	if (bind)
		fAddBinding({ {}, msg, 0, 0, BindType::echo });
	else
		util::nullLogger.log(msg);
}
void sys::Debugger::printEval(const std::u8string& msg, const std::u8string& value, bool hex, bool bind) {
	Expression parsed;
	if (!fParseExpression(value, parsed))
		return;

	if (bind)
		fAddBinding({ parsed, msg, 0, 0, (hex ? BindType::evalDec : BindType::evalHex) });
	else
		fPrintEval(parsed, msg, hex);
}
void sys::Debugger::printState(bool bind) {
	if (bind)
		fAddBinding({ {}, u8"",0, 0, BindType::state });
	else
		fPrintState();
}
void sys::Debugger::printInstructions(std::optional<std::u8string> address, size_t count, bool bind) {
	Expression parsed;
	if (!fParseExpression(address.value_or(u8"pc"), parsed))
		return;

	/* add the binding or print the state now */
	if (bind)
		fAddBinding({ parsed, u8"", 0, count, BindType::inst });
	else
		fPrintInstructions(parsed, count);
}
void sys::Debugger::printData8(const std::u8string& address, size_t bytes, bool bind) {
	Expression parsed;
	if (!fParseExpression(address, parsed))
		return;

	/* add the binding or print the state now */
	if (bind)
		fAddBinding({ parsed, u8"", 0, bytes, BindType::data8 });
	else
		fPrintData(parsed, bytes, 1);
}
void sys::Debugger::printData16(const std::u8string& address, size_t bytes, bool bind) {
	Expression parsed;
	if (!fParseExpression(address, parsed))
		return;

	/* add the binding or print the state now */
	if (bind)
		fAddBinding({ parsed, u8"", 0, bytes, BindType::data16 });
	else
		fPrintData(parsed, bytes, 2);
}
void sys::Debugger::printData32(const std::u8string& address, size_t bytes, bool bind) {
	Expression parsed;
	if (!fParseExpression(address, parsed))
		return;

	/* add the binding or print the state now */
	if (bind)
		fAddBinding({ parsed, u8"", 0, bytes, BindType::data32 });
	else
		fPrintData(parsed, bytes, 4);
}
void sys::Debugger::printData64(const std::u8string& address, size_t bytes, bool bind) {
	Expression parsed;
	if (!fParseExpression(address, parsed))
		return;

	/* add the binding or print the state now */
	if (bind)
		fAddBinding({ parsed, u8"", 0, bytes, BindType::data64 });
	else
		fPrintData(parsed, bytes, 8);
}
