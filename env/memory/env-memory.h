#pragma once

#include "../env-common.h"

namespace env {
	namespace bridge {
		struct Memory;
	}

	using addr_t = uint64_t;
	using physical_t = uint32_t;

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
		i32,
		i64,
		f32,
		f64
	};

	class Memory {
		friend struct bridge::Memory;
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
		env::Environment* pEnvironment{ 0 };
		uint32_t pCacheCount{ 0 };
		uint32_t pReadCache{ 0 };
		uint32_t pWriteCache{ 0 };
		uint32_t pExecuteCache{ 0 };
		uint32_t pCachePages{ 0 };
		std::u8string pRootModuleName;

	private:
		Memory(env::Environment& environment, std::u8string moduleName, uint32_t cacheSize);

	private:
		void fMakeAddress(wasm::Sink& sink, const env::MemoryState& state, uint32_t cache, const wasm::Variable& i64Address, const wasm::Function& lookup, env::MemoryType type) const;
		void fMakeLookup(const wasm::Memory& caches, const wasm::Function& function, const wasm::Function& lookup, uint32_t uasge) const;
		void fMakeRead(wasm::Sink& sink, const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const;
		void fMakeWrite(wasm::Sink& sink, const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const;
		void fMakeExecute(wasm::Sink& sink, const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const;
		void fMakeAccess(wasm::Module& mod, const env::MemoryState& state, const wasm::Prototype& readPrototype, const wasm::Prototype& writePrototype, std::u8string_view name, env::MemoryType type) const;

	private:
		env::MemoryRegion fLookup(env::addr_t address, uint32_t size, uint32_t usage);
		uint32_t fReadi32Fromi8(env::addr_t address) const;
		uint32_t fReadi32Fromu8(env::addr_t address) const;
		uint32_t fReadi32Fromi16(env::addr_t address) const;
		uint32_t fReadi32Fromu16(env::addr_t address) const;
		uint32_t fReadi32(env::addr_t address) const;
		uint64_t fReadi64(env::addr_t address) const;
		float fReadf32(env::addr_t address) const;
		double fReadf64(env::addr_t address) const;
		void fWritei32Fromi8(env::addr_t address, uint32_t value) const;
		void fWritei32Fromu8(env::addr_t address, uint32_t value) const;
		void fWritei32Fromi16(env::addr_t address, uint32_t value) const;
		void fWritei32Fromu16(env::addr_t address, uint32_t value) const;
		void fWritei32(env::addr_t address, uint32_t value) const;
		void fWritei64(env::addr_t address, uint64_t value) const;
		void fWritef32(env::addr_t address, float value) const;
		void fWritef64(env::addr_t address, double value) const;
		uint32_t fExecutei32Fromi8(env::addr_t address) const;
		uint32_t fExecutei32Fromu8(env::addr_t address) const;
		uint32_t fExecutei32Fromi16(env::addr_t address) const;
		uint32_t fExecutei32Fromu16(env::addr_t address) const;
		uint32_t fExecutei32(env::addr_t address) const;
		uint64_t fExecutei64(env::addr_t address) const;
		float fExecutef32(env::addr_t address) const;
		double fExecutef64(env::addr_t address) const;

	public:
		env::MemoryState setupCoreModule(wasm::Module& mod) const;
		env::MemoryState setupImports(wasm::Module& mod) const;
		void makeRead(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const;
		void makeWrite(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const;
		void makeExecute(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const;

	public:
		template <class Type>
		Type read(env::addr_t address) const {
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
		void write(env::addr_t address, Type value) {
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
		Type execute(env::addr_t address) const {
			static_assert(std::is_arithmetic_v<Type>, "Can only execute arithmetic types");
			if constexpr (std::is_same_v<Type, float>)
				return fExecutef32(address);
			else if constexpr (std::is_same_v<Type, double>)
				return fExecutef64(address);

			static constexpr bool sign = std::is_signed_v<Type>;
			switch (sizeof(Type)) {
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
			}
		};
	};
}
