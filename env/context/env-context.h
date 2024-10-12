#pragma once

#include "../env-common.h"
#include "context-access.h"
#include "context-bridge.h"

namespace env {
	class Context {
		friend struct bridge::Context;
		friend class detail::ContextInteract;
	private:
		std::function<void(bool)> pCoreLoaded;
		std::function<void(env::guest_t)> pTranslate;
		std::u8string pName;
		std::u8string pLogHeader;
		std::u8string pSelfName;
		env::Process* pProcess = 0;
		env::id_t pId = 0;

	public:
		Context(std::u8string_view name, env::Process* process);
		Context(env::Context&&) = delete;
		Context(const env::Context&) = delete;
		~Context();

	private:
		bool fCreate(std::function<void(env::guest_t)> translate);
		bool fSetCore(const uint8_t* data, size_t size, std::function<void(bool)> callback);
		void fCoreLoaded(bool succeeded);
		void fTranslate(env::guest_t address);

	public:
		const std::u8string& name() const;
		const std::u8string& logHeader() const;
		const std::u8string& selfName() const;
		wasm::Import imported() const;
		env::id_t id() const;
	};
}
