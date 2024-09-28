#pragma once

#include "env-common.h"
#include "env-process.h"

namespace env {
	struct MemoryRegion {
		env::addr_t address{ 0 };
		env::physical_t physical{ 0 };
		uint32_t size{ 0 };
	};
	struct MemoryState {
		wasm::Memory memory;
		wasm::Memory caches;
		wasm::Function readFunction;
		wasm::Function writeFunction;
		wasm::Function executeFunction;
	};
	enum class MemoryType : uint8_t {
		u8To32,
		u16To32,
		u8To64,
		u16To64,
		u32To64,
		i8To32,
		i16To32,
		i8To64,
		i16To64,
		i32To64,
		u32,
		u64,
		f32,
		f64
	};

	class Memory {
	private:
		struct Usage {
			static constexpr uint32_t Read = 0x01;
			static constexpr uint32_t Write = 0x02;
			static constexpr uint32_t Execute = 0x04;
		};
		struct Region : public env::MemoryRegion {
			uint32_t usage{ 0 };
			struct {
				env::physical_t prev = 0;
				env::physical_t next = 0;
			} physList;
			struct {
				env::physical_t prev = 0;
				env::physical_t next = 0;
			} addrList;
		};
		struct MemCache {
			env::addr_t address{ 0 };
			env::physical_t physical{ 0 };
			uint32_t size1{ 0 };
			uint32_t size2{ 0 };
			uint32_t size4{ 0 };
			uint32_t size8{ 0 };
		};

	private:
		env::Process* pProcess{ 0 };
		uint32_t pCacheCount{ 0 };
		uint32_t pCachePages{ 0 };

	private:
		Memory(env::Process* process, uint32_t cacheSize);

	private:
		void fMakeAddress(wasm::Sink& sink, const env::MemoryState& state, uint32_t cache, const wasm::Variable& i64Address, const wasm::Function& lookup, env::MemoryType type) const;
		void fMakeLookup(const wasm::Memory& caches, const wasm::Function& function, const wasm::Function& lookup, uint32_t uasge) const;

	public:
		env::MemoryState makeExport(wasm::Module& mod, const wasm::Function& lookup) const;
		env::MemoryState makeImport(wasm::Module& mod, std::u8string moduleName) const;
		void makeRead(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const;
		void makeWrite(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const;
		void makeExecute(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const;

	public:


	public:
		env::MemoryRegion lookup(env::addr_t addr, uint32_t size, uint32_t usage);
	};
}
