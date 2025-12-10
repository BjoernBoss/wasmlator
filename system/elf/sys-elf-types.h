/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../sys-common.h"

namespace sys {
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

		/* load-state describing the result of the load-operation */
		struct LoadState {
			std::u8string interpreter;
			struct {
				env::guest_t pageSize = 0;
				env::guest_t entry = 0;
				env::guest_t phAddress = 0;
				env::guest_t base = 0;
				size_t phEntrySize = 0;
				size_t phCount = 0;
			} aux;
			env::guest_t endOfData = 0;
			env::guest_t start = 0;
			elf::MachineType machine = elf::MachineType::none;
			uint8_t bitWidth = 0;

		};

		/* exception thrown for any issues regarding elf-file parsing/loading */
		struct Exception : public str::u8::BuildException {
			template <class... Args>
			constexpr Exception(const Args&... args) : str::u8::BuildException{ args... } {}
		};
	}

	namespace detail {
		enum class VersionType : uint32_t {
			none = 0,
			current = 1
		};
		enum class ProgramType : uint32_t {
			null = 0,
			load = 1,
			dynamic = 2,
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

		template <class Base>
		struct ElfHeader {
			uint8_t identifier[16];
			elf::ElfType type;
			elf::MachineType machine;
			detail::VersionType version;
			Base entry;
			Base phOffset;
			Base shOffset;
			uint32_t flags;
			uint16_t ehsize;
			uint16_t phEntrySize;
			uint16_t phCount;
			uint16_t shEntrySize;
			uint16_t shCount;
			uint16_t shStringIndex;
		};

		struct ProgramHeader32 {
			detail::ProgramType type;
			uint32_t offset;
			uint32_t vAddress;
			uint32_t pAddress;
			uint32_t fileSize;
			uint32_t memSize;
			uint32_t flags;
			uint32_t align;
		};
		struct ProgramHeader64 {
			detail::ProgramType type;
			uint32_t flags;
			uint64_t offset;
			uint64_t vAddress;
			uint64_t pAddress;
			uint64_t fileSize;
			uint64_t memSize;
			uint64_t align;
		};
		template <class Base>
		using ProgramHeader = std::conditional_t<std::is_same_v<Base, uint64_t>, detail::ProgramHeader64, detail::ProgramHeader32>;

		template <class Base>
		struct SectionHeader {
			uint32_t name;
			detail::SectionType type;
			Base flags;
			Base address;
			Base offset;
			Base size;
			uint32_t link;
			uint32_t info;
			Base addressAlign;
			Base entrySize;
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
					throw elf::Exception{ u8"Cannot read [", count, u8"] bytes from elf of size [", pSize, u8']' };
			}

		public:
			size_t size() const {
				return pSize;
			}
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
}
