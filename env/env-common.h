#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <string>
#include <type_traits>

#include "../util/logging.h"
#include "../wasm/wasm.h"

namespace env {
	using environment_t = uint32_t;

	static constexpr uint64_t PageSize = 0x1000;
	static constexpr uint32_t MinPages = 5'000;
	static constexpr uint32_t PageCount(uint64_t bytes) {
		return uint32_t((bytes + env::PageSize - 1) / env::PageSize);
	}

	class Environment {
	private:
		std::u8string pName;
		std::u8string pLogHeader;
		env::environment_t pIdentifier{ 0 };

	public:
		Environment(std::u8string_view name);

	public:
		const std::u8string& name() const;
		env::environment_t identifier() const;

	public:
		template <class... Args>
		void log(const Args&... args) const {
			util::log(pLogHeader, args...);
		}
		template <class... Args>
		void fail(const Args&... args) const {
			util::fail(pLogHeader, args...);
		}
	};
}
