#pragma once

#include "../env-common.h"
#include "mapping-access.h"
#include "mapping-bridge.h"

namespace env {
	namespace detail {
		static constexpr uint32_t BlockLookupCacheBits = 10;

		struct MappingCache {
			env::guest_t address = 0;
			uint32_t index = 0;
		};
	}

	class Mapping {
		friend struct detail::MappingBridge;
		friend struct detail::MappingAccess;
	private:
		std::unordered_map<env::guest_t, uint32_t> pMapping;
		uint32_t pCacheAddress = 0;

	public:
		Mapping() = default;
		Mapping(env::Mapping&&) = delete;
		Mapping(const env::Mapping&) = delete;

	private:
		uint32_t fResolve(env::guest_t address) const;
		void fAssociate(env::guest_t address, uint32_t index);
		void fFlushed();

	public:
		env::ExecState execute(env::guest_t address);
		bool contains(env::guest_t address) const;
		void flush();
	};
}
