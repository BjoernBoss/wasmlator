#include "sys-elf.h"

static util::Logger logger{ u8"sys::elf" };

bool sys::elf::CheckValid(const uint8_t* data, size_t size) {
	if (size < sizeof(detail::ElfHeader<uint32_t>))
		return false;

	/* validate the header identification structure */
	if (data[0] != detail::ident::_0Magic || data[1] != detail::ident::_1Magic || data[2] != detail::ident::_2Magic || data[3] != detail::ident::_3Magic)
		return false;
	if (data[4] != detail::ident::_4elfClass32 && data[4] != detail::ident::_4elfClass64)
		return false;
	if (data[5] != detail::ident::_5elfData2LSB)
		return false;
	if (data[6] != detail::ident::_6versionCurrent)
		return false;
	if (data[7] != detail::ident::_7osABILinux && data[7] != detail::ident::_7osABISYSV)
		return false;
	if (data[8] != detail::ident::_8osABIVersion)
		return false;
	return true;
}
sys::elf::ElfType sys::elf::GetType(const uint8_t* data, size_t size) {
	if (size < sizeof(detail::ElfHeader<uint32_t>))
		return elf::ElfType::none;
	const detail::ElfHeader<uint32_t>* header = reinterpret_cast<const detail::ElfHeader<uint32_t>*>(data);
	return header->type;
}
sys::elf::MachineType sys::elf::GetMachine(const uint8_t* data, size_t size) {
	if (size < sizeof(detail::ElfHeader<uint32_t>))
		return elf::MachineType::none;
	const detail::ElfHeader<uint32_t>* header = reinterpret_cast<const detail::ElfHeader<uint32_t>*>(data);
	return header->machine;
}
uint32_t sys::elf::GetBitWidth(const uint8_t* data, size_t size) {
	if (size < sizeof(detail::ElfHeader<uint32_t>))
		return 0;
	const detail::ElfHeader<uint32_t>* header = reinterpret_cast<const detail::ElfHeader<uint32_t>*>(data);
	if (header->identifier[4] == detail::ident::_4elfClass32)
		return 32;
	if (header->identifier[4] == detail::ident::_4elfClass64)
		return 64;
	return 0;
}

sys::elf::LoadState sys::LoadElf(const uint8_t* data, size_t size) {
	detail::Reader reader{ data, size };

	/* validate the raw file-header (bit-width agnostic) */
	if (!elf::CheckValid(data, size))
		throw elf::Exception{ L"Data do not have a valid elf-signature" };
	uint32_t bitWidth = elf::GetBitWidth(data, size);

	/* validate the sized elf-header */
	detail::ElfHeaderOut out;
	env::guest_t phAddress = 0;
	if (bitWidth == 32) {
		logger.debug(u8"Loading static 32bit elf binary");
		out = detail::ValidateElfHeader(reader.get<detail::ElfHeader<uint32_t>>(0), reader);
		phAddress = detail::CheckProgramHeaders<uint32_t>(out, reader);
	}
	else {
		logger.debug(u8"Loading static 64bit elf binary");
		out = detail::ValidateElfHeader(reader.get<detail::ElfHeader<uint64_t>>(0), reader);
		phAddress = detail::CheckProgramHeaders<uint64_t>(out, reader);
	}

	return {};
}
