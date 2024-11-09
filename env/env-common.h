#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <string>
#include <type_traits>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <bit>

#include "../interface/host.h"

namespace env {
	enum class ExecState : uint32_t {
		/* used internally for returns and such, may or may not have been translated yet, payload: next-address-to-execute */
		_execute,

		/* next address has not yet been translated, payload: none */
		translate,

		/* process has terminated, payload: none */
		terminated,

		/* custom exec-states, payload: none */
		_custom
	};

	static constexpr uint64_t VirtPageSize = 0x1000;
	static constexpr uint32_t VirtPageOffset(uint64_t address) {
		return uint32_t(address & (env::VirtPageSize - 1));
	}
	static constexpr uint64_t VirtPageIndex(uint64_t address) {
		return (address / env::VirtPageSize);
	}
	static constexpr uint32_t VirtPageCount(uint64_t bytes) {
		return uint32_t((bytes + env::VirtPageSize - 1) / env::VirtPageSize);
	}
	static constexpr uint64_t VirtPageAligned(uint64_t bytes) {
		return env::VirtPageSize * ((bytes + env::VirtPageSize - 1) / env::VirtPageSize);
	}

	static constexpr uint64_t PhysPageSize = 0x10000;
	static constexpr uint32_t PhysPageOffset(uint64_t address) {
		return uint32_t(address & (env::PhysPageSize - 1));
	}
	static constexpr uint64_t PhysPageIndex(uint64_t address) {
		return (address / env::PhysPageSize);
	}
	static constexpr uint32_t PhysPageCount(uint64_t bytes) {
		return uint32_t((bytes + env::PhysPageSize - 1) / env::PhysPageSize);
	}
	static constexpr uint64_t PhysPageAligned(uint64_t bytes) {
		return env::PhysPageSize * ((bytes + env::PhysPageSize - 1) / env::PhysPageSize);
	}

	using guest_t = uint64_t;
	using physical_t = uint32_t;

	class Process;

	/* memory page usage flags */
	struct MemoryUsage {
		static constexpr uint32_t Read = 0x01;
		static constexpr uint32_t Write = 0x02;
		static constexpr uint32_t Execute = 0x04;
	};

	/* block exported function entry */
	struct BlockExport {
		std::u8string name;
		env::guest_t address = 0;
	};
}
