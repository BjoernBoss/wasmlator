#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <type_traits>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <bit>
#include <memory>
#include <limits>
#include <exception>

#include "../interface/host.h"

namespace env {
	class Process;

	namespace detail {
		using physical_t = uint32_t;

		static constexpr uint64_t PhysPageSize = 0x10000;
		static constexpr uint32_t PhysPageOffset(uint64_t address) {
			return uint32_t(address & (detail::PhysPageSize - 1));
		}
		static constexpr uint64_t PhysPageIndex(uint64_t address) {
			return (address / detail::PhysPageSize);
		}
		static constexpr uint32_t PhysPageCount(uint64_t bytes) {
			return uint32_t((bytes + detail::PhysPageSize - 1) / detail::PhysPageSize);
		}
		static constexpr uint64_t PhysPageAligned(uint64_t bytes) {
			return detail::PhysPageSize * ((bytes + detail::PhysPageSize - 1) / detail::PhysPageSize);
		}
	}

	using guest_t = uint64_t;

	static constexpr uint64_t PageSize = 0x1000;
	static constexpr uint32_t PageOffset(uint64_t address) {
		return uint32_t(address & (env::PageSize - 1));
	}
	static constexpr uint64_t VirtPageIndex(uint64_t address) {
		return (address / env::PageSize);
	}
	static constexpr uint32_t VirtPageCount(uint64_t bytes) {
		return uint32_t((bytes + env::PageSize - 1) / env::PageSize);
	}
	static constexpr uint64_t VirtPageAligned(uint64_t bytes) {
		return env::PageSize * ((bytes + env::PageSize - 1) / env::PageSize);
	}

	/* memory page usage flags */
	struct MemoryUsage {
		static constexpr uint32_t Read = 0x01;
		static constexpr uint32_t Write = 0x02;
		static constexpr uint32_t Execute = 0x04;
		static constexpr uint32_t ReadWrite = MemoryUsage::Read | MemoryUsage::Write;
		static constexpr uint32_t ReadExecute = MemoryUsage::Read | MemoryUsage::Execute;
		static constexpr uint32_t All = MemoryUsage::Read | MemoryUsage::Write | MemoryUsage::Execute;
	};

	/* block exported function entry */
	struct BlockExport {
		std::u8string name;
		env::guest_t address = 0;
	};

	/* thrown the guest terminates */
	struct Termianted {
		int32_t code = 0;
	};

	/* thrown on memory exceptions (both when accessed by main or guest) */
	struct MemoryFault {
		env::guest_t address = 0;
		uint32_t size = 0;
		uint32_t usedUsage = 0;
		uint32_t actualUsage = 0;
	};

	/* thrown whenever an unknown address is to be executed */
	struct Translate {
		env::guest_t address = 0;
	};

	/* thrown whenever an undecodable instruction is to be executed */
	struct NotDecodable {
		env::guest_t address = 0;
	};
}
