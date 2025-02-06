#pragma once

#include <ustring/ustring.h>
#include <jsonify/jsonify.h>
#include <wasgen/wasm.h>
#include <cinttypes>
#include <type_traits>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <bit>
#include <memory>
#include <limits>
#include <optional>
#include <algorithm>
#include <vector>
#include <map>
#include <iterator>

#include "../util/util-logger.h"
#include "../util/util-path.h"

namespace env {
	class Process;

	namespace detail {
		static constexpr uint64_t PhysPageSize = 0x10000;
		static constexpr uint64_t PhysPageCount(uint64_t bytes) {
			return uint64_t((bytes + detail::PhysPageSize - 1) / detail::PhysPageSize);
		}
		static constexpr uint64_t PhysMaxPages = detail::PhysPageCount(std::numeric_limits<uint32_t>::max());
	}

	using guest_t = uint64_t;

	/* memory page usage flags */
	struct Usage {
		static constexpr uint32_t None = 0x00;
		static constexpr uint32_t Usages = 0x0f;
		static constexpr uint32_t Read = 0x01;
		static constexpr uint32_t Write = 0x02;
		static constexpr uint32_t Execute = 0x04;
		static constexpr uint32_t Lock = 0x08;
		static constexpr uint32_t ReadWrite = Usage::Read | Usage::Write;
		static constexpr uint32_t ReadExecute = Usage::Read | Usage::Execute;
		static constexpr uint32_t RWE = Usage::Read | Usage::Write | Usage::Execute;

		struct Print {
		private:
			str::u8::Local<4> pLocal;

		public:
			constexpr Print(uint32_t usage) {
				pLocal.push_back((usage & env::Usage::Read) ? u8'r' : u8'-');
				pLocal.push_back((usage & env::Usage::Write) ? u8'w' : u8'-');
				pLocal.push_back((usage & env::Usage::Execute) ? u8'x' : u8'-');
				pLocal.push_back((usage & env::Usage::Lock) ? u8'l' : u8'-');
			}

		public:
			constexpr str::u8::Local<4> str() const {
				return pLocal;
			}
		};
	};

	/* block exported function entry */
	struct BlockExport {
		std::u8string name;
		env::guest_t address = 0;
	};

	/* system interface is used to setup and configure the environment accordingly and interact with it
	*	Note: wasm should only be generated within setupCore/setupBlock, as they are wrapped to catch any potential wasm-issues */
	class System {
	protected:
		System() = default;

	public:
		virtual ~System() = default;

	public:
		/* invoked once when the core module is supposed to be configured */
		virtual bool setupCore(wasm::Module& mod) = 0;

		/* invoked when the configured core module has been loaded */
		virtual void coreLoaded() = 0;

		/* invoked for every new block module to be configured, after being triggered to be created */
		virtual std::vector<env::BlockExport> setupBlock(wasm::Module& mod) = 0;

		/* invoked when the last configured block has been loaded */
		virtual void blockLoaded() = 0;

		/* invoked when the system should tear itself down */
		virtual void shutdown() = 0;
	};

	/* base of exceptions thrown by the execution environment */
	struct Exception {
	public:
		env::guest_t address = 0;

	protected:
		Exception(env::guest_t address) : address{ address } {}
	};

	/* thrown when the guest terminates */
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
		uint64_t size = 0;
		uint32_t usedUsage = 0;
		uint32_t actualUsage = 0;

	public:
		MemoryFault(env::guest_t address, env::guest_t accessed, uint64_t size, uint32_t usedUsage, uint32_t actualUsage) :
			env::Exception{ address }, accessed{ accessed }, size{ size }, usedUsage{ usedUsage }, actualUsage{ actualUsage } {}
	};

	/* thrown whenever executable data have been cleared or modified */
	struct ExecuteDirty : public env::Exception {
	public:
		ExecuteDirty(env::guest_t address) : env::Exception{ address } {}
	};

	/* thrown whenever an unknown address is to be executed */
	struct Translate : public env::Exception {
	public:
		Translate(env::guest_t address) : env::Exception{ address } {}
	};

	/* thrown whenever an undecodable instruction is to be executed */
	struct Decoding : public env::Exception {
	public:
		bool memoryFault = false;

	public:
		Decoding(env::guest_t address, bool memoryFault) : env::Exception{ address }, memoryFault{ memoryFault } {}
	};
}

/* format-helper for usage-printer */
template <> struct str::Formatter<env::Usage::Print> {
	constexpr bool operator()(str::IsSink auto& sink, const env::Usage::Print& val, std::u32string_view fmt) const {
		return str::CallFormat(sink, val.str(), fmt);
	}
};
