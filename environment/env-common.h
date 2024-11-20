#pragma once

#include <ustring/ustring.h>
#include <wasgen/wasm.h>
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

	/* system interface is used to setup and configure the environment accordingly and interact with it
	*	Note: wasm should only be generated within setupCore/setupBlock, as they are wrapped to catch any potential wasm-issues */
	class System {
	private:
		uint32_t pPageSize = 0;
		uint32_t pMemoryCaches = 0;
		uint32_t pContextSize = 0;

	protected:
		constexpr System(uint32_t pageSize, uint32_t memoryCaches, uint32_t contextSize) : pPageSize{ pageSize }, pMemoryCaches{ memoryCaches }, pContextSize{ contextSize } {}

	public:
		virtual ~System() = default;

	public:
		virtual void setupCore(wasm::Module& mod) = 0;
		virtual std::vector<env::BlockExport> setupBlock(wasm::Module& mod) = 0;
		virtual void coreLoaded() = 0;
		virtual void blockLoaded() = 0;

	public:
		constexpr uint32_t pageSize() const {
			return pPageSize;
		}
		constexpr uint32_t memoryCaches() const {
			return pMemoryCaches;
		}
		constexpr uint32_t contextSize() const {
			return pContextSize;
		}
	};
}
