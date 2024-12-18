#pragma once

#include "../env-common.h"
#include "mapping-access.h"
#include "mapping-bridge.h"

namespace env {
	namespace detail {
		static constexpr uint32_t BlockLookupCacheBits = 10;
		static constexpr uint32_t BlockCacheCount = (1 << detail::BlockLookupCacheBits);

		struct MappingCache {
			env::guest_t address = 0;
			uint32_t index = 0;
		};

		/* must be zero, as both EqualZero and memset(null) are used */
		static constexpr uint32_t InvalidMapping = 0;
	}

	class Mapping {
		friend struct detail::MappingBridge;
		friend struct detail::MappingAccess;
	private:
		std::unordered_map<env::guest_t, uint32_t> pMapping;
		detail::MappingCache pCaches[detail::BlockCacheCount];

	public:
		Mapping() = default;
		Mapping(env::Mapping&&) = delete;
		Mapping(const env::Mapping&) = delete;

	private:
		uint32_t fResolve(env::guest_t address) const;
		bool fCheckLoadable(const std::vector<env::BlockExport>& exports);
		void fBlockExports(const std::vector<env::BlockExport>& exports);

	public:
		env::guest_t execute(env::guest_t address);
		bool contains(env::guest_t address) const;
		void flush();
	};
}
