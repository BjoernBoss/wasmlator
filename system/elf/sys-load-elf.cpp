#include "sys-elf.h"

static util::Logger logger{ u8"sys::elf" };

template<class Base>
sys::detail::ElfConfig sys::detail::ValidateElfHeader(const detail::Reader& reader) {
	/* validate the type of this binary */
	const detail::ElfHeader<Base>* header = reader.get<detail::ElfHeader<Base>>(0);
	if (header->type != elf::ElfType::executable && header->type != elf::ElfType::dynamic)
		throw elf::Exception{ L"Elf is not executable" };

	/* validate the remaining parameter of the header */
	if (header->version != detail::VersionType::current)
		throw elf::Exception{ L"Invalid version" };
	if (header->entry == 0)
		throw elf::Exception{ L"No valid entry-point found" };
	if (header->phOffset == 0 || header->phCount == 0 || header->phEntrySize != sizeof(detail::ProgramHeader<Base>))
		throw elf::Exception{ L"Issue with program-headers detected" };
	if (header->ehsize != sizeof(detail::ElfHeader<Base>))
		throw elf::Exception{ "Issue with executable-header detected" };

	/* setup the initial load-state */
	detail::ElfConfig config;
	config.entry = header->entry;
	config.phOffset = header->phOffset;
	config.phEntrySize = header->phEntrySize;
	config.machine = header->machine;
	config.dynamic = (header->type == elf::ElfType::dynamic);

	/* compute the number of program-headers */
	config.phCount = header->phCount;
	if (config.phCount == detail::consts::maxPhCount)
		config.phCount = reader.get<detail::SectionHeader<Base>>(header->shOffset)->info;
	return config;
}

template <class Base>
sys::detail::ElfConfig sys::detail::ValidateElfLoadTyped(const detail::Reader& reader) {
	/* validate the initial elf-header */
	detail::ElfConfig config = detail::ValidateElfHeader<Base>(reader);

	/* lookup the ph-list */
	const detail::ProgramHeader<Base>* phList = reader.base<detail::ProgramHeader<Base>>(config.phOffset, config.phCount);

	/* iterate over the program header and look for all interesting headers required for loading */
	bool loadFound = false, phFound = false, intFound = false;
	for (size_t i = 0; i < config.phCount; ++i) {
		/* check if the first loadable header has been found */
		if (phList[i].type == detail::ProgramType::load)
			loadFound = true;

		/* check if the interpreter has been found */
		else if (phList[i].type == detail::ProgramType::interpreter) {
			if (loadFound)
				throw elf::Exception{ L"Malformed program-headers with load entries before the interpreter entry" };
			if (intFound)
				throw elf::Exception{ L"Malformed program-headers with multiple interpreter entries" };
			intFound = true;

			/* validate the interpreter path (must be a null-terminated absolute path) */
			std::u8string_view path = { reader.base<char8_t>(phList[i].offset, phList[i].fileSize), size_t(phList[i].fileSize) };
			if (path.find(u8'\0') != path.size() - 1 || util::TestPath(path) != util::PathState::absolute)
				throw elf::Exception{ L"Invalid interpreter path encountered" };
			config.interpreter = path.substr(0, path.size() - 1);
		}

		/* check if the headers themselves have been found */
		else if (phList[i].type == detail::ProgramType::programHeader) {
			if (loadFound)
				throw elf::Exception{ L"Malformed program-headers with load entries before the program-header entry" };
			if (phFound)
				throw elf::Exception{ L"Malformed program-headers with multiple program-header entries" };
			phFound = true;
			config.phAddress = phList[i].vAddress;
		}
	}
	return config;
}

template <class Base>
env::guest_t sys::detail::LoadElfSingleProgHeader(env::guest_t baseAddress, size_t index, const detail::ProgramHeader<Base>& header, const detail::Reader& reader) {
	env::guest_t pageSize = env::Instance()->pageSize();

	/* validate the alignment */
	if (header.align > 1 && (pageSize % header.align) != 0)
		throw elf::Exception{ L"Program-header alignment is not aligned with page alignment" };

	/* construct the page-usage to be used */
	uint32_t usage = 0;
	if ((header.flags & detail::programFlags::exec) == detail::programFlags::exec)
		usage |= env::Usage::Execute;
	if ((header.flags & detail::programFlags::read) == detail::programFlags::read)
		usage |= env::Usage::Read;
	if ((header.flags & detail::programFlags::write) == detail::programFlags::write)
		usage |= env::Usage::Write;

	/* compute the adjusted vitual address */
	env::guest_t virtAddress = header.vAddress + baseAddress;

	/* compute the page-aligned address and size */
	env::guest_t address = (virtAddress & ~(pageSize - 1));
	uint64_t size = (((virtAddress + header.memSize + pageSize - 1) & ~(pageSize - 1)) - address);
	if (header.fileSize > header.memSize)
		throw elf::Exception{ L"Program-header contains larger file-size than memory-size" };

	/* allocate the memory (start it out as writable) */
	logger.fmtDebug(u8"Mapping program-header [{}] to [{:#018x}] with size [{:#018x}] (actual: [{:#018x}] with size [{:#018x}]) with usage [{}{}{}]",
		index, address, size, virtAddress, header.memSize,
		(usage & env::Usage::Read ? u8'r' : u8'-'),
		(usage & env::Usage::Write ? u8'w' : u8'-'),
		(usage & env::Usage::Execute ? u8'x' : u8'-'));
	if (!env::Instance()->memory().mmap(address, size, usage))
		throw elf::Exception{ L"Failed to allocate memory for program-header [", index, L']' };

	/* write the actual data to the section (no usage to ensure it cannot fail - as the flags have already been applied) */
	const uint8_t* data = reader.base<uint8_t>(header.offset, header.fileSize);
	env::Instance()->memory().mwrite(virtAddress, data, header.fileSize, env::Usage::None);

	/* return the end-of-data address */
	return (address + size);
}

template <class Base>
env::guest_t sys::detail::LoadElfProgHeadersTyped(env::guest_t baseAddress, const detail::ElfConfig& config, const detail::Reader& reader) {
	/* extract the list of program-headers */
	const detail::ProgramHeader<Base>* phList = reader.base<detail::ProgramHeader<Base>>(config.phOffset, config.phCount);

	/* iterate over the headers and load them and accumulate the end-of-data address */
	env::guest_t endOfData = 0;
	for (size_t i = 0; i < config.phCount; ++i) {
		if (phList[i].type == detail::ProgramType::load)
			endOfData = std::max<env::guest_t>(endOfData, detail::LoadElfSingleProgHeader<Base>(baseAddress, i, phList[i], reader));
	}
	return endOfData;
}

uint8_t sys::detail::CheckElfSignature(const detail::Reader& reader) {
	if (reader.size() < sizeof(detail::ElfHeader<uint32_t>))
		return 0;
	const uint8_t* data = reader.base<uint8_t>(0, 9);

	/* validate the header identification structure */
	if (data[0] != detail::ident::_0Magic || data[1] != detail::ident::_1Magic || data[2] != detail::ident::_2Magic || data[3] != detail::ident::_3Magic)
		return 0;
	if (data[4] != detail::ident::_4elfClass32 && data[4] != detail::ident::_4elfClass64)
		return 0;
	if (data[5] != detail::ident::_5elfData2LSB)
		return 0;
	if (data[6] != detail::ident::_6versionCurrent)
		return 0;
	if (data[7] != detail::ident::_7osABILinux && data[7] != detail::ident::_7osABISYSV)
		return 0;
	if (data[8] != detail::ident::_8osABIVersion)
		return 0;

	/* return the bit-width of the file */
	return (data[4] == detail::ident::_4elfClass32 ? 32 : 64);
}

sys::detail::ElfConfig sys::detail::ValidateElfLoad(const detail::Reader& reader, uint8_t bitWidth) {
	if (bitWidth == 32)
		return detail::ValidateElfLoadTyped<uint32_t>(reader);
	return detail::ValidateElfLoadTyped<uint64_t>(reader);
}

env::guest_t sys::detail::LoadElfProgHeaders(env::guest_t baseAddress, const detail::ElfConfig& config, const detail::Reader& reader, uint8_t bitWidth) {
	if (bitWidth == 32)
		return detail::LoadElfProgHeadersTyped<uint32_t>(baseAddress, config, reader);
	return detail::LoadElfProgHeadersTyped<uint64_t>(baseAddress, config, reader);
}
