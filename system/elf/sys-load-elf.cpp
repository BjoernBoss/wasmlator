#include "sys-elf.h"

static util::Logger logger{ u8"sys::elf" };

template<class Base>
sys::detail::ElfHeaderOut sys::detail::ValidateElfHeader(const detail::ElfHeader<Base>* header, const detail::Reader& reader) {
	/* validate the type of this binary */
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
	detail::ElfHeaderOut out;
	out.start = header->entry;
	out.phOffset = header->phOffset;
	out.phSize = header->phEntrySize;
	out.dynamic = (header->type == elf::ElfType::dynamic);

	/* compute the number of program-headers */
	out.phCount = header->phCount;
	if (out.phCount == detail::consts::maxPhCount)
		out.phCount = reader.get<detail::SectionHeader<Base>>(header->shOffset)->info;
	return out;
}

template <class Base>
env::guest_t sys::detail::CheckProgramHeaders(const detail::ElfHeaderOut& config, const detail::Reader& reader) {
	bool loadFound = false;
	size_t phIndex = config.phCount, intIndex = config.phCount;

	/* lookup the ph-list */
	const detail::ProgramHeader<Base>* phList = reader.base<detail::ProgramHeader<Base>>(config.phOffset, config.phCount);

	/* iterate over the program header and look for header-entries themselves as well as for a potential interpreter */
	for (size_t i = 0; i < config.phCount; ++i) {
		/* check if the first loadable header has been found */
		if (phList[i].type == detail::ProgramType::load)
			loadFound = true;

		/* check if the interpreter has been found */
		else if (phList[i].type == detail::ProgramType::interpreter) {
			if (loadFound)
				throw elf::Exception{ L"Malformed program-headers with load entries before the interpreter entry" };
			if (intIndex != config.phCount)
				throw elf::Exception{ L"Malformed program-headers with multiple interpreter entries" };
			intIndex = i;
		}

		/* check if the headers themselves have been found */
		else if (phList[i].type == detail::ProgramType::programHeader) {
			if (loadFound)
				throw elf::Exception{ L"Malformed program-headers with load entries before the program-header entry" };
			if (phIndex != config.phCount)
				throw elf::Exception{ L"Malformed program-headers with multiple program-header entries" };
			phIndex = i;
		}
	}

	/* check if no interpreter is required and the loading can be continued */
	if (intIndex == config.phCount)
		return (phIndex < config.phCount ? phList[phIndex].vAddress : 0);

	/* extract the interpreter path */
	std::u8string_view path = { reader.base<char8_t>(phList[intIndex].offset, phList[intIndex].fileSize), size_t(phList[intIndex].fileSize) };
	if (path.ends_with(u8'\0'))
		path = path.substr(0, path.size() - 1);

	/* notify about the required interpreter */
	if (path.find(u8'\0') != std::u8string_view::npos || util::TestPath(path) != util::PathState::absolute)
		throw elf::Exception{ L"Invalid interpreter path encountered" };
	throw elf::Interpreter{ std::u8string(path) };
}

/* explicit template instantiations */
template sys::detail::ElfHeaderOut sys::detail::ValidateElfHeader<uint32_t>(const detail::ElfHeader<uint32_t>*, const detail::Reader&);
template sys::detail::ElfHeaderOut sys::detail::ValidateElfHeader<uint64_t>(const detail::ElfHeader<uint64_t>*, const detail::Reader&);
template env::guest_t sys::detail::CheckProgramHeaders<uint32_t>(const detail::ElfHeaderOut&, const detail::Reader&);
template env::guest_t sys::detail::CheckProgramHeaders<uint64_t>(const detail::ElfHeaderOut&, const detail::Reader&);
