#include "sys-elf.h"

bool sys::elf::CheckValid(const uint8_t* data, size_t size) {
	if (size < sizeof(elf::ElfHeader<uint32_t>))
		return false;

	/* validate the header identification structure */
	if (data[0] != elf::ident::_0Magic || data[1] != elf::ident::_1Magic || data[2] != elf::ident::_2Magic || data[3] != elf::ident::_3Magic)
		return false;
	if (data[4] != elf::ident::_4elfClass32 && data[4] != elf::ident::_4elfClass64)
		return false;
	if (data[5] != elf::ident::_5elfData2LSB)
		return false;
	if (data[6] != elf::ident::_6versionCurrent)
		return false;
	if (data[7] != elf::ident::_7osABILinux && data[7] != elf::ident::_7osABISYSV)
		return false;
	if (data[8] != elf::ident::_8osABIVersion)
		return false;
	return true;
}
sys::elf::ElfType sys::elf::GetType(const uint8_t* data, size_t size) {
	if (size < sizeof(elf::ElfHeader<uint32_t>))
		return elf::ElfType::none;
	const elf::ElfHeader<uint32_t>* header = reinterpret_cast<const elf::ElfHeader<uint32_t>*>(data);
	return header->type;
}
sys::elf::MachineType sys::elf::GetMachine(const uint8_t* data, size_t size) {
	if (size < sizeof(elf::ElfHeader<uint32_t>))
		return elf::MachineType::none;
	const elf::ElfHeader<uint32_t>* header = reinterpret_cast<const elf::ElfHeader<uint32_t>*>(data);
	return header->machine;
}
uint32_t sys::elf::GetBitWidth(const uint8_t* data, size_t size) {
	if (size < sizeof(elf::ElfHeader<uint32_t>))
		return 0;
	const elf::ElfHeader<uint32_t>* header = reinterpret_cast<const elf::ElfHeader<uint32_t>*>(data);
	if (header->identifier[4] == ident::_4elfClass32)
		return 32;
	if (header->identifier[4] == ident::_4elfClass64)
		return 64;
	return 0;
}
