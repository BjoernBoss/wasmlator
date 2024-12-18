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
	struct Usage {
		static constexpr uint32_t Read = 0x01;
		static constexpr uint32_t Write = 0x02;
		static constexpr uint32_t Execute = 0x04;
		static constexpr uint32_t ReadWrite = Usage::Read | Usage::Write;
		static constexpr uint32_t ReadExecute = Usage::Read | Usage::Execute;
		static constexpr uint32_t All = Usage::Read | Usage::Write | Usage::Execute;
	};

	/* global generation-configuration of the process */
	struct GenConfig {
		size_t translationDepth = 4;
		bool singleStep = false;
		bool logBlocks = false;
	};

	/* block exported function entry */
	struct BlockExport {
		std::u8string name;
		env::guest_t address = 0;
	};

	/* system interface is used to setup and configure the environment accordingly and interact with it
	*	Note: wasm should only be generated within setupCore/setupBlock, as they are wrapped to catch any potential wasm-issues
	*	Note: PageSize must be greater than zero and a power of two */
	class System {
	private:
		uint32_t pPageSize = 0;
		uint32_t pMemoryCaches = 0;
		uint32_t pContextSize = 0;

	protected:
		System(uint32_t pageSize, uint32_t memoryCaches, uint32_t contextSize) : pPageSize{ pageSize }, pMemoryCaches{ memoryCaches }, pContextSize{ contextSize } {
			if (pPageSize == 0 || ((pPageSize - 1) & pPageSize) != 0)
				host::Logger{ u8"env::system" }.fatal(u8"PageSize must be greater than zero and a power of two");
		}

	public:
		virtual ~System() = default;

	public:
		/* invoked once when the core module is supposed to be configured */
		virtual void setupCore(wasm::Module& mod) = 0;

		/* invoked for every new block module to be configured, after being triggered to be created */
		virtual std::vector<env::BlockExport> setupBlock(wasm::Module& mod) = 0;

		/* invoked when the configured core module has been loaded */
		virtual void coreLoaded() = 0;

		/* invoked when the last configured block has been loaded */
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

	/* base of exceptions thrown by the execution environment */
	struct Exception {
	public:
		env::guest_t address = 0;

	protected:
		Exception(env::guest_t address) : address{ address } {}
	};

	/* thrown the guest terminates */
	struct Terminated : public env::Exception {
	public:
		int32_t code = 0;

	public:
		Terminated(env::guest_t address, int32_t code) : env::Exception{ address }, code{ code } {}
	};

	/* thrown on memory exceptions (both when accessed by main or guest) */
	struct MemoryFault : public env::Exception {
	public:
		env::guest_t accessed = 0;
		uint32_t size = 0;
		uint32_t usedUsage = 0;
		uint32_t actualUsage = 0;

	public:
		MemoryFault(env::guest_t address, env::guest_t accessed, uint32_t size, uint32_t usedUsage, uint32_t actualUsage) :
			env::Exception{ address }, accessed{ accessed }, size{ size }, usedUsage{ usedUsage }, actualUsage{ actualUsage } {}
	};

	/* thrown whenever an unknown address is to be executed */
	struct Translate : public env::Exception {
	public:
		Translate(env::guest_t address) : env::Exception{ address } {}
	};

	/* thrown whenever an undecodable instruction is to be executed */
	struct NotDecodable : public env::Exception {
	public:
		NotDecodable(env::guest_t address) : env::Exception{ address } {}
	};

	/* thrown whenever a single step has been taken in single-step configuration */
	struct SingleStep : public env::Exception {
	public:
		SingleStep(env::guest_t address) : env::Exception{ address } {}
	};
}
