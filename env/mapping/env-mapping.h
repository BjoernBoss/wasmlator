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
		enum class LoadingState : uint32_t {
			ready,
			busy
		};

		/* must be zero, as MappingWriter::makeCheckFailed uses wasm::EqualZero internally */
		static constexpr uint32_t NotFoundIndex = 0;
	}

	class Mapping {
		friend struct detail::MappingBridge;
		friend struct detail::MappingAccess;
	private:
		std::function<void()> pLoaded;
		std::vector<env::BlockExport> pExports;
		std::unordered_map<env::guest_t, uint32_t> pMapping;
		uint32_t pCacheAddress = 0;
		uint32_t pLoadingAddress = 0;

	public:
		Mapping() = default;
		Mapping(env::Mapping&&) = delete;
		Mapping(const env::Mapping&) = delete;

	private:
		uint32_t fResolve(env::guest_t address) const;
		void fBlockLoaded(bool succeeded);
		void fFlushed();

	public:
		void loadBlock(const uint8_t* data, size_t size, const std::vector<env::BlockExport>& exports, std::function<void()> callback);
		env::ExecState execute(env::guest_t address);
		bool contains(env::guest_t address) const;
		void flush();
	};
}
