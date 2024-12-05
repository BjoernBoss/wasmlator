#include "elf-static.h"

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
static void UnpackElfFile(const elf::Reader& reader) {
	using ElfType = elf::ElfHeader<BaseType>;
	using SecType = elf::SectionHeader<BaseType>;
	using ProType = elf::ProgramHeader<BaseType>;
	const ElfType* header = reader.get<ElfType>(0);

	/* fetch the number of program-headers */
	size_t programHeaderCount = header->phCount;
	if (programHeaderCount == elf::consts::maxPhCount)
		programHeaderCount = reader.get<SecType>(header->shOffset)->info;
	const ProType* programs = reader.base<ProType>(header->phOffset, programHeaderCount);

	/* fetch the number of section-headers */
	size_t sectionHeaderCount = header->shCount;
	if (sectionHeaderCount == elf::consts::maxShCount)
		sectionHeaderCount = reader.get<SecType>(header->shOffset)->size;
	const SecType* sections = reader.base<SecType>(header->shOffset, sectionHeaderCount);

	/* fetch the index of the string-section */
	size_t stringIndex = header->shStringIndex;
	if (stringIndex == elf::consts::maxStrIndex)
		stringIndex = reader.get<SecType>(header->shOffset)->link;
	if (stringIndex != elf::consts::noStrIndex && stringIndex >= sectionHeaderCount)
		throw elf::Exception{ L"String section index [", stringIndex, L"] out of bounds [", sectionHeaderCount, L"]" };

	/* lookup the bss-section in in the list of sections */
	const SecType* bssSection = 0;
	if (stringIndex != elf::consts::noStrIndex) {
		const SecType* strings = sections + stringIndex;
		const char* base = reader.base<char>(strings->offset, strings->size);

		/* iterate over the sections and lookup the section with the given name */
		for (size_t i = 0; i < sectionHeaderCount; ++i) {
			const SecType* section = sections + i;
			if (section->name + 5 > strings->size)
				continue;
			if (std::string_view{ ".bss" }.compare(base + section->name) != 0)
				continue;
			bssSection = section;
			break;
		}
	}

	/* check if the binary requires an interpreter, in which case it is not considered static */
	for (size_t i = 0; i < programHeaderCount; ++i) {
		if (programs[i].type == elf::ProgramType::interpreter)
			throw elf::Exception{ L"Interpreter required to load binary" };
	}

	/* write all program-headers to be loaded to the guest environment */
	env::guest_t pageSize = env::Instance()->system().pageSize();
	for (size_t i = 0; i < programHeaderCount; ++i) {
		if (programs[i].type != elf::ProgramType::load)
			continue;

		/* construct the page-usage to be used */
		uint32_t usage = 0;
		if ((programs[i].flags & elf::programFlags::exec) == elf::programFlags::exec)
			usage |= env::MemoryUsage::Execute;
		if ((programs[i].flags & elf::programFlags::read) == elf::programFlags::read)
			usage |= env::MemoryUsage::Read;
		if ((programs[i].flags & elf::programFlags::write) == elf::programFlags::write)
			usage |= env::MemoryUsage::Write;

		/* compute the page-aligned address and size */
		env::guest_t address = (programs[i].vAddress & ~(pageSize - 1));
		uint32_t size = uint32_t(((programs[i].vAddress + programs[i].memSize + pageSize - 1) & ~(pageSize - 1)) - address);
		if (programs[i].fileSize > programs[i].memSize)
			throw elf::Exception{ L"Program header contains larger file-size than memory-size" };

		/* allocate the memory (start it out as writable) */
		host::Debug(str::u8::Format(u8"Mapping program-header [{}] to [{:#018x}] with size [{:#018x}] with usage [{}{}{}]", i, address, size,
			(usage & env::MemoryUsage::Read ? u8'r' : u8'-'),
			(usage & env::MemoryUsage::Write ? u8'w' : u8'-'),
			(usage & env::MemoryUsage::Execute ? u8'x' : u8'-')));
		if (!env::Instance()->memory().mmap(address, size, env::MemoryUsage::Write))
			throw elf::Exception{ L"Failed to allocate memory for program-header [", i, L']' };
		env::Instance()->memory().mclear(address, size, env::MemoryUsage::Write);

		/* write the actual data to the section */
		const uint8_t* data = reader.base<uint8_t>(programs[i].offset, programs[i].fileSize);
		env::Instance()->memory().mwrite(address, data, uint32_t(programs[i].fileSize), env::MemoryUsage::Write);

		/* update the protection flags to match the actual flags */
		env::Instance()->memory().mprotect(address, size, usage);
	}

	/* setup the bss section */
	if (bssSection != 0) {
		/* allocate the page-aligned bss-section data */
		env::guest_t address = (bssSection->address & ~(pageSize - 1));
		uint32_t size = uint32_t(((bssSection->address + bssSection->size + pageSize - 1) & ~(pageSize - 1)) - address);
		if (!env::Instance()->memory().mmap(address, size, env::MemoryUsage::ReadWrite))
			throw elf::Exception{ L"Failed to allocate memory for [.bss] section" };

		/* clear the bss-section memory */
		env::Instance()->memory().mclear(address, size, env::MemoryUsage::Write);
	}
}

template <class BaseType>
static env::guest_t LoadStaticElf(const elf::Reader& reader) {
	const elf::ElfHeader<BaseType>* header = reader.get<elf::ElfHeader<BaseType>>(0);

	/* validate the overall elf-header and unpack the elf-file */
	ValidateElfHeader<BaseType>(header);
	UnpackElfFile<BaseType>(reader);

	/* return the entry-point */
	return header->entry;
}

env::guest_t elf::LoadStatic(const uint8_t* data, size_t size) {
	elf::Reader reader{ data, size };

	/* validate the raw file-header (bit-width agnostic) */
	uint32_t bitWidth = ValidateElfIdentifier(data, size);

	/* parse the elf-file based on its used bit-width */
	if (bitWidth == 32) {
		host::Debug(u8"Loading static 32bit elf binary");
		return LoadStaticElf<uint32_t>(reader);
	}
	host::Debug(u8"Loading static 64bit elf binary");
	return LoadStaticElf<uint64_t>(reader);
}
