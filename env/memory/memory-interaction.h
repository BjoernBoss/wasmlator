#pragma once

#include "memory-common.h"

namespace env::detail {
	class MemoryInteraction {
		friend struct bridge::Memory;
	private:
		static constexpr uint32_t InternalCaches = 3;

	private:
		struct MemCache {
			env::guest_t address{ 0 };
			env::physical_t physical{ 0 };
			uint32_t size1{ 0 };
			uint32_t size2{ 0 };
			uint32_t size4{ 0 };
			uint32_t size8{ 0 };
		};

	private:
		env::Process* pProcess = 0;
		uint32_t pCacheCount = 0;
		uint32_t pReadCache = 0;
		uint32_t pWriteCache = 0;
		uint32_t pExecuteCache = 0;
		uint32_t pCacheAddress = 0;

	public:
		MemoryInteraction(env::Process* process, uint32_t cacheSize);
		MemoryInteraction(detail::MemoryInteraction&&) = delete;
		MemoryInteraction(const detail::MemoryInteraction&) = delete;

	private:
		void fCheckCache(uint32_t cache) const;
		void fMakeAddress(wasm::Sink& sink, const env::ModuleState& state, uint32_t cache, const wasm::Variable& i64Address, const wasm::Function& lookup, env::MemoryType type) const;
		void fMakeLookup(const env::CoreState& state, const wasm::Function& function, uint32_t usage) const;
		void fMakeRead(wasm::Sink& sink, const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cache, env::MemoryType type) const;
		void fMakeWrite(wasm::Sink& sink, const wasm::Variable& i64Address, const wasm::Variable& value, const env::ModuleState& state, uint32_t cache, env::MemoryType type) const;
		void fMakeExecute(wasm::Sink& sink, const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cache, env::MemoryType type) const;
		void fMakeAccess(wasm::Module& mod, const env::CoreState& state, wasm::Type type, std::u8string_view name, env::MemoryType memoryType) const;

	private:
		uint32_t fReadi32Fromi8(env::guest_t address) const;
		uint32_t fReadi32Fromu8(env::guest_t address) const;
		uint32_t fReadi32Fromi16(env::guest_t address) const;
		uint32_t fReadi32Fromu16(env::guest_t address) const;
		uint32_t fReadi32(env::guest_t address) const;
		uint64_t fReadi64(env::guest_t address) const;
		float fReadf32(env::guest_t address) const;
		double fReadf64(env::guest_t address) const;
		void fWritei32Fromi8(env::guest_t address, uint32_t value) const;
		void fWritei32Fromu8(env::guest_t address, uint32_t value) const;
		void fWritei32Fromi16(env::guest_t address, uint32_t value) const;
		void fWritei32Fromu16(env::guest_t address, uint32_t value) const;
		void fWritei32(env::guest_t address, uint32_t value) const;
		void fWritei64(env::guest_t address, uint64_t value) const;
		void fWritef32(env::guest_t address, float value) const;
		void fWritef64(env::guest_t address, double value) const;
		uint32_t fExecutei32Fromi8(env::guest_t address) const;
		uint32_t fExecutei32Fromu8(env::guest_t address) const;
		uint32_t fExecutei32Fromi16(env::guest_t address) const;
		uint32_t fExecutei32Fromu16(env::guest_t address) const;
		uint32_t fExecutei32(env::guest_t address) const;
		uint64_t fExecutei64(env::guest_t address) const;
		float fExecutef32(env::guest_t address) const;
		double fExecutef64(env::guest_t address) const;

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state);
		void setupCoreBody(wasm::Module& mod, env::CoreState& state) const;
		void setupBlockImports(wasm::Module& mod, env::BlockState& state) const;

	public:
		void makeRead(const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const;
		void makeWrite(const wasm::Variable& i64Address, const wasm::Variable& value, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const;
		void makeExecute(const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const;

	public:
		template <class Type>
		Type read(env::guest_t address) const {
			static_assert(std::is_arithmetic_v<Type>, "Can only read arithmetic types");
			if constexpr (std::is_same_v<Type, float>)
				return fReadf32(address);
			else if constexpr (std::is_same_v<Type, double>)
				return fReadf64(address);

			static constexpr bool sign = std::is_signed_v<Type>;
			switch (sizeof(Type)) {
			case 1:
				if constexpr (sign)
					return Type(fReadi32Fromi8(address));
				else
					return Type(fReadi32Fromu8(address));
			case 2:
				if constexpr (sign)
					return Type(fReadi32Fromi16(address));
				else
					return Type(fReadi32Fromu16(address));
			case 4:
				return Type(fReadi32(address));
			case 8:
			default:
				return Type(fReadi64(address));
			}
		}
		template <class Type>
		void write(env::guest_t address, Type value) {
			static_assert(std::is_arithmetic_v<Type>, "Can only write arithmetic types");
			if constexpr (std::is_same_v<Type, float>)
				fWritef32(address, value);
			else if constexpr (std::is_same_v<Type, double>)
				fWritef64(address, value);
			else {
				static constexpr bool sign = std::is_signed_v<Type>;
				switch (sizeof(Type)) {
				case 1:
					if constexpr (sign)
						fWritei32Fromi8(address, uint32_t(value));
					else
						fWritei32Fromu8(address, uint32_t(value));
					break;
				case 2:
					if constexpr (sign)
						fWritei32Fromi16(address, uint32_t(value));
					else
						fWritei32Fromu16(address, uint32_t(value));
					break;
				case 4:
					fWritei32(address, uint32_t(value));
					break;
				case 8:
				default:
					fWritei64(address, uint64_t(value));
					break;
				}
			}
		}
		template <class Type>
		Type execute(env::guest_t address) const {
			static_assert(std::is_arithmetic_v<Type>, "Can only execute arithmetic types");
			if constexpr (std::is_same_v<Type, float>)
				return fExecutef32(address);
			else if constexpr (std::is_same_v<Type, double>)
				return fExecutef64(address);

			static constexpr bool sign = std::is_signed_v<Type>;
			switch (sizeof(Type)) {
			case 1:
				if constexpr (sign)
					return Type(fExecutei32Fromi8(address));
				else
					return Type(fExecutei32Fromu8(address));
			case 2:
				if constexpr (sign)
					return Type(fExecutei32Fromi16(address));
				else
					return Type(fExecutei32Fromu16(address));
			case 4:
				return Type(fExecutei32(address));
			case 8:
			default:
				return Type(fExecutei64(address));
			}
		};
	};
}
