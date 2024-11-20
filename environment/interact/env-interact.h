#pragma once

#include "../env-common.h"
#include "interact-access.h"
#include "interact-bridge.h"

namespace env {
	class Interact {
		friend struct detail::InteractBridge;
		friend struct detail::InteractAccess;

	private:
		std::unordered_map<std::u8string, uint32_t> pCallVoid;
		std::unordered_map<std::u8string, uint32_t> pCallParam;
		std::vector<std::function<void()>> pVoidCallbacks;
		std::vector<std::function<uint64_t(uint64_t)>> pParamCallbacks;
		bool pCoreLoaded = false;

	public:
		Interact() = default;
		Interact(env::Interact&&) = delete;
		Interact(const env::Interact&) = delete;

	private:
		bool fCheck(uint32_t index, bool param) const;
		void fInvoke(uint32_t index) const;
		uint64_t fInvoke(uint64_t param, uint32_t index) const;

	public:
		uint32_t defineCallback(std::function<void()> fn);
		uint32_t defineCallback(std::function<uint64_t(uint64_t)> fn);
		void call(const std::u8string& name) const;
		uint64_t call(const std::u8string& name, uint64_t param) const;
	};
}
