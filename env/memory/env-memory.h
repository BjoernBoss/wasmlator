#pragma once

#include "memory-common.h"
#include "memory-mapper.h"
#include "memory-interaction.h"

namespace env {
	class Memory {
		friend struct bridge::Memory;
	private:
		struct MemCache {
			env::addr_t address = 0;
			env::physical_t physical = 0;
			uint32_t size1 = 0;
			uint32_t size2 = 0;
			uint32_t size4 = 0;
			uint32_t size8 = 0;
		};

	private:
		detail::MemoryMapper pMapper;
		detail::MemoryInteraction pInteraction;

	public:
		Memory(env::Process* process, uint32_t cacheSize);
		Memory(env::Memory&&) = delete;
		Memory(const env::Memory&) = delete;

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state);
		void setupCoreBody(wasm::Module& mod, env::CoreState& state) const;
		void setupBlockImports(wasm::Module& mod, env::BlockState& state) const;

	public:
		void makeRead(const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const;
		void makeWrite(const wasm::Variable& i64Address, const wasm::Variable& value, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const;
		void makeExecute(const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const;

	public:
		bool mmap(env::addr_t address, uint32_t size, uint32_t usage);
		void munmap(env::addr_t address, uint32_t size);
		void mprotect(env::addr_t address, uint32_t size, uint32_t usage);
		template <class Type>
		Type read(env::addr_t address) const {
			return pInteraction.read<Type>(address);
		}
		template <class Type>
		void write(env::addr_t address, Type value) {
			return pInteraction.write<Type>(address, value);
		}
		template <class Type>
		Type execute(env::addr_t address) const {
			return pInteraction.execute<Type>(address);
		};
	};
}
