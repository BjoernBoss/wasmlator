#pragma once

#include "../env-common.h"
#include "memory-mapper.h"
#include "memory-interaction.h"
#include "memory-access.h"
#include "memory-bridge.h"

namespace env {
	class Memory {
		friend struct bridge::Memory;
		friend class detail::MemoryAccess;
	private:
		struct MemCache {
			env::guest_t address = 0;
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
		Memory(env::Process* process);
		Memory(env::Memory&&) = delete;
		Memory(const env::Memory&) = delete;

	public:
		bool mmap(env::guest_t address, uint32_t size, uint32_t usage);
		void munmap(env::guest_t address, uint32_t size);
		void mprotect(env::guest_t address, uint32_t size, uint32_t usage);
		template <class Type>
		Type read(env::guest_t address) const {
			return pInteraction.read<Type>(address);
		}
		template <class Type>
		void write(env::guest_t address, Type value) {
			return pInteraction.write<Type>(address, value);
		}
		template <class Type>
		Type code(env::guest_t address) const {
			return pInteraction.code<Type>(address);
		};
	};
}
