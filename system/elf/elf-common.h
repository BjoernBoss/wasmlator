#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <type_traits>

#include "../../environment/environment.h"

namespace elf {
	enum class ElfType : uint16_t {
		none = 0,
		relocatable = 1,
		executable = 2,
		dynamic = 3,
		core = 4
	};
	enum class MachineType : uint16_t {
		none = 0,
		x86_64 = 62,
		riscv = 243
	};
	enum class VersionType : uint32_t {
		none = 0,
		current = 1
	};
	enum class ProgramType : uint32_t {
		null = 0,
		load = 1,
		dymaic = 2,
		interpreter = 3,
		note = 4,
		programHeader = 6,
		tls = 7
	};
	enum class SectionType : uint32_t {
		null = 0,
		programBits = 1,
		symbolTable = 2,
		stringTable = 3,
		relocationAddends = 4,
		hash = 5,
		dynamic = 6,
		note = 7,
		noBits = 8,
		relocationNoAddends = 9,
		dynamicSymbols = 11
	};
	enum class AuxiliaryType : uint32_t {
		null = 0,
		ignore = 1,
		executableFd = 2,
		phAddress = 3,
		phEntrySize = 4,
		phCount = 5,
		pageSize = 6,
		baseInterpreter = 7,
		flags = 8,
		entry = 9,
		notELF = 10,
		uid = 11,
		euid = 12,
		gid = 13,
		egid = 14,
		clockTick = 17,
		secure = 23,
		random = 25,
		executableFilename = 31
	};

	namespace ident {
		static constexpr uint8_t _0Magic = 0x7f;
		static constexpr uint8_t _1Magic = 'E';
		static constexpr uint8_t _2Magic = 'L';
		static constexpr uint8_t _3Magic = 'F';
		static constexpr uint8_t _4elfClassNone = 0;
		static constexpr uint8_t _4elfClass32 = 1;
		static constexpr uint8_t _4elfClass64 = 2;
		static constexpr uint8_t _5elfDataNone = 0;
		static constexpr uint8_t _5elfData2LSB = 1;
		static constexpr uint8_t _5elfData2MSB = 2;
		static constexpr uint8_t _6versionNone = 0;
		static constexpr uint8_t _6versionCurrent = 1;
		static constexpr uint8_t _7osABINone = 0;
		static constexpr uint8_t _7osABISYSV = 0;
		static constexpr uint8_t _7osABILinux = 3;
		static constexpr uint8_t _8osABIVersion = 0;
	}

	namespace consts {
		static constexpr uint16_t maxPhCount = 0xffff;
		static constexpr uint16_t maxShCount = 0x0000;
		static constexpr uint16_t maxStrIndex = 0xffff;
		static constexpr uint16_t noStrIndex = 0x0000;
	}

	namespace programFlags {
		static constexpr uint32_t none = 0;
		static constexpr uint32_t exec = 1;
		static constexpr uint32_t write = 2;
		static constexpr uint32_t read = 4;
	}

	namespace sectionFlags {
		static constexpr uint32_t write = 1;
		static constexpr uint32_t alloc = 2;
		static constexpr uint32_t instructions = 4;
	}

	template <class BaseType>
	struct ElfHeader {
		uint8_t identifier[16];
		elf::ElfType type;
		elf::MachineType machine;
		elf::VersionType version;
		BaseType entry;
		BaseType phOffset;
		BaseType shOffset;
		uint32_t flags;
		uint16_t ehsize;
		uint16_t phEntrySize;
		uint16_t phCount;
		uint16_t shEntrySize;
		uint16_t shCount;
		uint16_t shStringIndex;
	};

	struct ProgramHeader32 {
		elf::ProgramType type;
		uint32_t offset;
		uint32_t vAddress;
		uint32_t pAddress;
		uint32_t fileSize;
		uint32_t memSize;
		uint32_t flags;
		uint32_t align;
	};
	struct ProgramHeader64 {
		elf::ProgramType type;
		uint32_t flags;
		uint64_t offset;
		uint64_t vAddress;
		uint64_t pAddress;
		uint64_t fileSize;
		uint64_t memSize;
		uint64_t align;
	};
	template <class BaseType>
	using ProgramHeader = std::conditional_t<std::is_same_v<BaseType, uint64_t>, elf::ProgramHeader64, elf::ProgramHeader32>;

	template <class BaseType>
	struct SectionHeader {
		uint32_t name;
		elf::SectionType type;
		BaseType flags;
		BaseType address;
		BaseType offset;
		BaseType size;
		uint32_t link;
		uint32_t info;
		BaseType addressAlign;
		BaseType entrySize;
	};

	/* fetch the elf-type from the given bytes or return none, if it does not exist */
	inline elf::ElfType GetType(const uint8_t* data, size_t size) {
		if (size < sizeof(elf::ElfHeader<uint32_t>))
			return elf::ElfType::none;
		const elf::ElfHeader<uint32_t>* header = reinterpret_cast<const elf::ElfHeader<uint32_t>*>(data);
		return header->type;
	}

	/* fetch the elf-machine type from the given bytes or return none, if it does not exist */
	inline elf::MachineType GetMachine(const uint8_t* data, size_t size) {
		if (size < sizeof(elf::ElfHeader<uint32_t>))
			return elf::MachineType::none;
		const elf::ElfHeader<uint32_t>* header = reinterpret_cast<const elf::ElfHeader<uint32_t>*>(data);
		return header->machine;
	}

	/* fetch the elf bit-width from the given bytes or return 0, if it does not exist */
	inline uint32_t GetBitWidth(const uint8_t* data, size_t size) {
		if (size < sizeof(elf::ElfHeader<uint32_t>))
			return 0;
		const elf::ElfHeader<uint32_t>* header = reinterpret_cast<const elf::ElfHeader<uint32_t>*>(data);
		if (header->identifier[4] == ident::_4elfClass32)
			return 32;
		if (header->identifier[4] == ident::_4elfClass64)
			return 64;
		return 0;
	}

	/* exception thrown for any issues regarding elf-file parsing/loading */
	struct Exception : public str::BuildException {
		template <class... Args>
		constexpr Exception(const Args&... args) : str::BuildException{ args... } {}
	};

	class Reader {
	private:
		const uint8_t* pData = 0;
		size_t pSize = 0;

	public:
		Reader(const uint8_t* data, size_t size) : pData{ data }, pSize{ size } {}

	private:
		void fCheck(size_t count) const {
			if (count > pSize)
				throw elf::Exception{ L"Cannot read [", count, L"] bytes from elf of size [", pSize, L"]" };
		}

	public:
		template <class Type>
		const Type* get(size_t offset) const {
			fCheck(offset + sizeof(Type));
			return reinterpret_cast<const Type*>(pData + offset);
		}
		template <class Type>
		const Type* base(size_t offset, size_t size) const {
			fCheck(offset + size * sizeof(Type));
			return reinterpret_cast<const Type*>(pData + offset);
		}
	};
}
