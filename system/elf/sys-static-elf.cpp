#include "sys-elf.h"

static util::Logger logger{ u8"sys::elf" };
//
//template <class BaseType>
//static std::pair<env::guest_t, env::guest_t> UnpackElfFile(const detail::Reader& reader) {
//	using ElfType = elf::ElfHeader<BaseType>;
//	using SecType = elf::SectionHeader<BaseType>;
//	using ProType = elf::ProgramHeader<BaseType>;
//	const ElfType* header = reader.get<ElfType>(0);
//
//	/* fetch the number of program-headers */
//	size_t programHeaderCount = header->phCount;
//	if (programHeaderCount == elf::consts::maxPhCount)
//		programHeaderCount = reader.get<SecType>(header->shOffset)->info;
//	const ProType* programs = reader.base<ProType>(header->phOffset, programHeaderCount);
//
//	/* lookup the address of the program-header and check if the binary requires an interpreter */
//	env::guest_t phAddress = 0;
//	bool loadFound = false, interpreterFound = false, programHeaderFound = false;
//	for (size_t i = 0; i < programHeaderCount; ++i) {
//		if (programs[i].type == elf::ProgramType::load)
//			loadFound = true;
//		else if (programs[i].type == elf::ProgramType::interpreter) {
//			if (loadFound)
//				throw elf::Exception{ L"Malformed program-headers with load entries before the interpreter entry" };
//			if (interpreterFound)
//				throw elf::Exception{ L"Malformed program-headers with multiple interpreter entries" };
//			interpreterFound = true;
//			throw elf::Exception{ L"Interpreter required to load binary" };
//		}
//		else if (programs[i].type == elf::ProgramType::programHeader) {
//			if (loadFound)
//				throw elf::Exception{ L"Malformed program-headers with load entries before the program-header entry" };
//			if (programHeaderFound)
//				throw elf::Exception{ L"Malformed program-headers with multiple program-header entries" };
//			programHeaderFound = true;
//			phAddress = programs[i].vAddress;
//		}
//	}
//
//	/* write all program-headers to be loaded to the guest environment and find the end of the data and address of the actual program-header table) */
//	env::guest_t pageSize = env::Instance()->pageSize();
//	env::guest_t endOfData = 0;
//	for (size_t i = 0; i < programHeaderCount; ++i) {
//		if (programs[i].type != elf::ProgramType::load)
//			continue;
//
//		/* validate the alignment */
//		if (programs[i].align > 1 && (pageSize % programs[i].align) != 0)
//			throw elf::Exception{ L"Program-header alignment is not aligned with page alignment" };
//
//		/* construct the page-usage to be used */
//		uint32_t usage = 0;
//		if ((programs[i].flags & elf::programFlags::exec) == elf::programFlags::exec)
//			usage |= env::Usage::Execute;
//		if ((programs[i].flags & elf::programFlags::read) == elf::programFlags::read)
//			usage |= env::Usage::Read;
//		if ((programs[i].flags & elf::programFlags::write) == elf::programFlags::write)
//			usage |= env::Usage::Write;
//
//		/* compute the page-aligned address and size */
//		env::guest_t address = (programs[i].vAddress & ~(pageSize - 1));
//		uint64_t size = (((programs[i].vAddress + programs[i].memSize + pageSize - 1) & ~(pageSize - 1)) - address);
//		if (programs[i].fileSize > programs[i].memSize)
//			throw elf::Exception{ L"Program-header contains larger file-size than memory-size" };
//
//		/* allocate the memory (start it out as writable) */
//		logger.fmtDebug(u8"Mapping program-header [{}] to [{:#018x}] with size [{:#018x}] (actual: [{:#018x}] with size [{:#018x}]) with usage [{}{}{}]",
//			i, address, size, programs[i].vAddress, programs[i].memSize,
//			(usage & env::Usage::Read ? u8'r' : u8'-'),
//			(usage & env::Usage::Write ? u8'w' : u8'-'),
//			(usage & env::Usage::Execute ? u8'x' : u8'-'));
//		if (!env::Instance()->memory().mmap(address, size, env::Usage::Write))
//			throw elf::Exception{ L"Failed to allocate memory for program-header [", i, L']' };
//
//		/* write the actual data to the section */
//		const uint8_t* data = reader.base<uint8_t>(programs[i].offset, programs[i].fileSize);
//		env::Instance()->memory().mwrite(programs[i].vAddress, data, programs[i].fileSize, env::Usage::Write);
//
//		/* update the protection flags to match the actual flags */
//		if (!env::Instance()->memory().mprotect(address, size, usage))
//			throw elf::Exception{ L"Failed to change the protection for program-header [", i, L']' };
//
//		/* update the end-of-data address */
//		endOfData = std::max(endOfData, address + size);
//	}
//	return { endOfData, phAddress };
//}
//
//template <class BaseType>
//static sys::ElfLoaded LoadStaticElf(const detail::Reader& reader) {
//	const elf::ElfHeader<BaseType>* header = reader.get<elf::ElfHeader<BaseType>>(0);
//
//	/* validate the overall elf-header and unpack the elf-file */
//	ValidateElfHeader<BaseType>(header);
//	auto [endOfData, phAddress] = UnpackElfFile<BaseType>(reader);
//
//	/* finalize the output state and return it */
//	sys::ElfLoaded output;
//	output.start = header->entry;
//	output.phCount = header->phCount;
//	output.phEntrySize = header->phEntrySize;
//	output.endOfData = endOfData;
//	output.phAddress = phAddress;
//	return output;
//}
