#include "sys-static-elf.h"

static host::Logger logger{ u8"sys::Elf" };

namespace elf = sys::elf;

static uint32_t ValidateElfIdentifier(const uint8_t* data, size_t size) {
	if (size < 9)
		throw elf::Exception{ L"Buffer too small to contain elf identifier" };

	/* validate the header identification structure */
	if (data[0] != elf::ident::_0Magic || data[1] != elf::ident::_1Magic || data[2] != elf::ident::_2Magic || data[3] != elf::ident::_3Magic)
		throw elf::Exception{ L"Invalid elf magic identifier" };
	if (data[4] != elf::ident::_4elfClass32 && data[4] != elf::ident::_4elfClass64)
		throw elf::Exception{ L"Unsupported bit-width [only 32 and 64 bit supported]" };
	if (data[5] != elf::ident::_5elfData2LSB)
		throw elf::Exception{ L"Only little endianness supported" };
	if (data[6] != elf::ident::_6versionCurrent)
		throw elf::Exception{ L"Invalid current version" };
	if (data[7] != elf::ident::_7osABILinux && data[7] != elf::ident::_7osABISYSV)
		throw elf::Exception{ L"Unsupported OS-ABI" };
	if (data[8] != elf::ident::_8osABIVersion)
		throw elf::Exception{ L"Unsupported abi-version" };

	/* return the bit-width */
	return (data[4] == elf::ident::_4elfClass32 ? 32 : 64);
}

template <class BaseType>
static void ValidateElfHeader(const elf::ElfHeader<BaseType>* header) {
	/* validate the type of this binary */
	if (header->type != elf::ElfType::executable)
		throw elf::Exception{ L"Not a static executable" };

	/* validate the remaining parameter of the header */
	if (header->version != elf::VersionType::current)
		throw elf::Exception{ L"Invalid version" };
	if (header->entry == 0)
		throw elf::Exception{ L"Invalid entry-point" };
	if (header->phOffset == 0 || header->phCount == 0 || header->phEntrySize != sizeof(elf::ProgramHeader<BaseType>))
		throw elf::Exception{ L"Issue with program-headers detected" };
	if (header->ehsize != sizeof(elf::ElfHeader<BaseType>))
		throw elf::Exception{ "Issue with executable-header detected" };
}

template <class BaseType>
static std::pair<env::guest_t, env::guest_t> UnpackElfFile(const elf::Reader& reader) {
	using ElfType = elf::ElfHeader<BaseType>;
	using SecType = elf::SectionHeader<BaseType>;
	using ProType = elf::ProgramHeader<BaseType>;
	const ElfType* header = reader.get<ElfType>(0);

	/* fetch the number of program-headers */
	size_t programHeaderCount = header->phCount;
	if (programHeaderCount == elf::consts::maxPhCount)
		programHeaderCount = reader.get<SecType>(header->shOffset)->info;
	const ProType* programs = reader.base<ProType>(header->phOffset, programHeaderCount);

	/* check if the binary requires an interpreter, in which case it is not considered static */
	for (size_t i = 0; i < programHeaderCount; ++i) {
		if (programs[i].type == elf::ProgramType::interpreter)
			throw elf::Exception{ L"Interpreter required to load binary" };
	}

	/* write all program-headers to be loaded to the guest environment and find the end of the data and address of the actual program-header table) */
	env::guest_t pageSize = env::Instance()->pageSize();
	env::guest_t endOfData = 0, phAddress = 0;
	for (size_t i = 0; i < programHeaderCount; ++i) {
		if (programs[i].type != elf::ProgramType::load)
			continue;

		/* validate the alignment */
		if (programs[i].align > 1 && (pageSize % programs[i].align) != 0)
			throw elf::Exception{ L"Program-header alignment is not aligned with page alignment" };

		/* construct the page-usage to be used */
		uint32_t usage = 0;
		if ((programs[i].flags & elf::programFlags::exec) == elf::programFlags::exec)
			usage |= env::Usage::Execute;
		if ((programs[i].flags & elf::programFlags::read) == elf::programFlags::read)
			usage |= env::Usage::Read;
		if ((programs[i].flags & elf::programFlags::write) == elf::programFlags::write)
			usage |= env::Usage::Write;

		/* compute the page-aligned address and size */
		env::guest_t address = (programs[i].vAddress & ~(pageSize - 1));
		uint32_t size = uint32_t(((programs[i].vAddress + programs[i].memSize + pageSize - 1) & ~(pageSize - 1)) - address);
		if (programs[i].fileSize > programs[i].memSize)
			throw elf::Exception{ L"Program-header contains larger file-size than memory-size" };

		/* allocate the memory (start it out as writable) */
		logger.fmtDebug(u8"Mapping program-header [{}] to [{:#018x}] with size [{:#018x}] (actual: [{:#018x}] with size [{:#018x}]) with usage [{}{}{}]",
			i, address, size, programs[i].vAddress, programs[i].memSize,
			(usage & env::Usage::Read ? u8'r' : u8'-'),
			(usage & env::Usage::Write ? u8'w' : u8'-'),
			(usage & env::Usage::Execute ? u8'x' : u8'-'));
		if (!env::Instance()->memory().mmap(address, size, env::Usage::Write))
			throw elf::Exception{ L"Failed to allocate memory for program-header [", i, L']' };

		/* write the actual data to the section */
		const uint8_t* data = reader.base<uint8_t>(programs[i].offset, programs[i].fileSize);
		env::Instance()->memory().mwrite(programs[i].vAddress, data, uint32_t(programs[i].fileSize), env::Usage::Write);

		/* update the protection flags to match the actual flags */
		if (!env::Instance()->memory().mprotect(address, size, usage))
			throw elf::Exception{ L"Failed to change the protection for program-header [", i, L']' };

		/* update the end-of-data address and check if this block contained the program header table */
		endOfData = std::max(endOfData, address + size);
		if (programs[i].offset <= header->phOffset && programs[i].offset + programs[i].fileSize >= header->phOffset + programHeaderCount * sizeof(ProType))
			phAddress = programs[i].vAddress + (header->phOffset - programs[i].offset);
	}
	return { endOfData, phAddress };
}

template <class BaseType>
static sys::ElfLoaded LoadStaticElf(const elf::Reader& reader) {
	const elf::ElfHeader<BaseType>* header = reader.get<elf::ElfHeader<BaseType>>(0);

	/* validate the overall elf-header and unpack the elf-file */
	ValidateElfHeader<BaseType>(header);
	auto [endOfData, phAddress] = UnpackElfFile<BaseType>(reader);

	/* finalize the output state and return it */
	sys::ElfLoaded output;
	output.start = header->entry;
	output.phCount = header->phCount;
	output.phEntrySize = header->phEntrySize;
	output.endOfData = endOfData;
	output.is64Bit = (sizeof(BaseType) == 8);
	output.phAddress = phAddress;
	return output;
}

sys::ElfLoaded sys::LoadElfStatic(const uint8_t* data, size_t size) {
	elf::Reader reader{ data, size };

	/* validate the raw file-header (bit-width agnostic) */
	uint32_t bitWidth = ValidateElfIdentifier(data, size);

	/* parse the elf-file based on its used bit-width */
	if (bitWidth == 32) {
		logger.debug(u8"Loading static 32bit elf binary");
		return LoadStaticElf<uint32_t>(reader);
	}
	logger.debug(u8"Loading static 64bit elf binary");
	return LoadStaticElf<uint64_t>(reader);
}
