#pragma once

#include "../env-common.h"

namespace env {
	namespace bridge {
		struct Context;
	}

	using id_t = uint32_t;

	class Context {
		friend struct bridge::Context;
	private:
		std::function<void(bool)> pCoreLoaded;
		env::Process* pProcess = 0;
		std::u8string pName;
		std::u8string pLogHeader;
		std::u8string pSelfName;
		env::id_t pId = 0;

	public:
		Context(std::u8string_view name, env::Process* process);
		Context(env::Context&&) = delete;
		Context(const env::Context&) = delete;
		~Context();

	private:
		void fCoreLoaded(bool succeeded);

	public:
		bool create();
		bool setCore(const uint8_t* data, size_t size, std::function<void(bool)> callback);
		const std::u8string& name() const;
		const std::u8string& selfName() const;
		const std::u8string& logHeader() const;
		env::id_t id() const;
	};
}
