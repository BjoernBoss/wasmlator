#include "sys-elf.h"

static util::Logger logger{ u8"sys::elf" };

sys::elf::LoadState sys::LoadElf(const uint8_t* data, size_t size) {
	detail::Reader reader{ data, size };

	/* validate the fundamental elf-signature */
	uint8_t bitWidth = detail::CheckElfSignature(reader);
	if (bitWidth == 0)
		throw elf::Exception{ L"Data do not have a valid elf-signature" };
	logger.debug(u8"Loading ", bitWidth, u8" bit elf binary");

	/* validate the initial elf header and all of the program headers */
	detail::ElfConfig config = detail::ValidateElfLoad(reader, bitWidth);

	/* check if a base-address needs to be picked (sufficiently far away from start of large allocations-address) */
	env::guest_t baseAddress = 0, pageSize = env::Instance()->pageSize();
	if (config.dynamic)
		baseAddress = env::guest_t(1 + (host::GetRandom() & 0x00ff'ffff)) * pageSize;
	logger.debug(u8"Selecting base-address as: ", str::As{ U"#018x", baseAddress });

	/* load all program headers to memory */
	detail::LoadElfProgHeaders(baseAddress, config, reader, bitWidth);

	/* setup the loaded state */
	elf::LoadState loaded;
	std::swap(loaded.interpreter, config.interpreter);
	loaded.start = config.entry + baseAddress;
	loaded.endOfData = config.endOfData;
	loaded.machine = config.machine;
	loaded.bitWidth = bitWidth;
	loaded.aux.entry = loaded.start;
	loaded.aux.phAddress = config.phAddress + baseAddress;
	loaded.aux.phCount = config.phCount;
	loaded.aux.phEntrySize = config.phEntrySize;
	loaded.aux.pageSize = pageSize;
	loaded.aux.base = 0;
	return loaded;
}

void sys::LoadElfInterpreter(elf::LoadState& state, const uint8_t* data, size_t size) {
	detail::Reader reader{ data, size };

	/* validate the fundamental elf-signature */
	uint8_t bitWidth = detail::CheckElfSignature(reader);
	if (bitWidth == 0)
		throw elf::Exception{ L"Data do not have a valid elf-signature" };

	/* check if the bit-width matches */
	if (bitWidth != state.bitWidth)
		throw elf::Exception{ L"Interpreter does not match the word-width of the application" };
	logger.debug(u8"Loading ", bitWidth, u8" bit elf interpreter");

	/* validate the initial elf header and all of the program headers */
	detail::ElfConfig config = detail::ValidateElfLoad(reader, bitWidth);

	/* check if the machine type matches */
	if (config.machine != state.machine)
		throw elf::Exception{ L"Interpreter does not match the machine of the application" };

	/* validate the initial configuration */
	if (!config.interpreter.empty())
		throw elf::Exception{ L"Interpreter expects recursive interpreter" };

	/* check if a base-address needs to be picked (move it far behind the main application) */
	env::guest_t baseAddress = 0, pageSize = env::Instance()->pageSize();
	if (config.dynamic) {
		baseAddress = 0x0000'0400'0000'0000 + (state.endOfData & env::guest_t(pageSize - 1));
		baseAddress += env::guest_t(host::GetRandom() & 0x00ff'ffff) * pageSize;
	}
	logger.debug(u8"Selecting base-address for interpreter as: ", str::As{ U"#018x", baseAddress });

	/* load all program headers to memory (discard end-of-data, as the previous end-of-data value is being used) */
	detail::LoadElfProgHeaders(baseAddress, config, reader, bitWidth);

	/* patch the final state */
	state.aux.base = baseAddress;
	state.start = config.entry + baseAddress;
}
