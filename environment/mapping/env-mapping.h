#pragma once

#include "../env-common.h"
#include "mapping-access.h"
#include "mapping-bridge.h"

namespace env {
	namespace detail {
		static constexpr uint32_t BlockFastCacheBits = 10;
		static constexpr uint32_t BlockFastCount = (1 << detail::BlockFastCacheBits);

		struct MappingCache {
			env::guest_t address = 0;
			uint32_t index = 0;
		};

		/* must be zero, as memset(null) are used */
		static constexpr uint32_t InvalidMapping = 0;
	}

	/* maximum number of super-block mappings allowed, before a force-flush of all blocks is performed */
	static constexpr uint32_t MaxMappingCount = 0x4000;

	/* maximum number of loaded block-modules allowed, before a force-flush of all blocks is performed */
	static constexpr uint32_t MaxBlockCount = 0x0200;

	class Mapping {
		friend struct detail::MappingBridge;
		friend struct detail::MappingAccess;
	private:
		std::unordered_map<env::guest_t, uint32_t> pMapping;
		detail::MappingCache pFastCache[detail::BlockFastCount];
		size_t pTotalBlockCount = 0;

	public:
		Mapping() = default;
		Mapping(env::Mapping&&) = delete;
		Mapping(const env::Mapping&) = delete;

	private:
		void fFlush();
		void fCheckFlush();
		uint32_t fResolve(env::guest_t address);
		void fCheckLoadable(const std::vector<env::BlockExport>& exports);
		void fBlockExports(const std::vector<env::BlockExport>& exports);

	public:
		env::guest_t execute(env::guest_t address);
		bool contains(env::guest_t address) const;
		void flush();
	};
}
