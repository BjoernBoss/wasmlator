#pragma once

#include "../env-common.h"
#include "context-access.h"
#include "context-bridge.h"

namespace env {
	class Context {
		friend struct bridge::Context;
		friend class detail::ContextAccess;
	private:
		std::function<void(bool)> pLoaded;
		std::function<void(int32_t)> pTerminated;
		std::function<void(env::guest_t)> pTranslate;
		std::vector<env::BlockExport> pExports;
		std::u8string pName;
		std::u8string pLogHeader;
		env::Process* pProcess = 0;
		env::id_t pId = 0;

	public:
		Context(std::u8string_view name, env::Process* process);
		Context(env::Context&&) = delete;
		Context(const env::Context&) = delete;
		~Context();

	private:
		bool fCreate(std::function<void(env::guest_t)> translate, std::function<void(int32_t)> terminated);
		void fCoreLoaded(bool succeeded);
		void fBlockLoaded(bool succeeded);
		void fTranslate(env::guest_t address);
		void fTerminated(int32_t code);

	public:
		void loadCore(const uint8_t* data, size_t size, std::function<void(bool)> callback);
		void loadBlock(const uint8_t* data, size_t size, const std::vector<env::BlockExport>& exports, std::function<void(bool)> callback);
		const std::u8string& name() const;
		const std::u8string& logHeader() const;
		env::id_t id() const;

	public:
		void makeExit(const wasm::Variable& i32ExitCode, const env::ModuleState& state) const;
	};
}
