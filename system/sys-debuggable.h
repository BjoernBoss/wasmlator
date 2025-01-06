#pragma once

#include <unordered_map>
#include <string>

#include "../environment/environment.h"

namespace sys {
	class Debuggable {
	protected:
		Debuggable() = default;

	public:
		virtual ~Debuggable() = default;

	public:
		/* read the current cpu state */
		virtual void getState(std::unordered_map<std::u8string, uintptr_t>& state) const = 0;

		/* get the current pc */
		virtual env::guest_t getPC() const = 0;

		/* decode the instruction at the address */
		virtual std::u8string decode(uintptr_t address) const = 0;

		/* set a value of the current cpu state */
		virtual void setValue(std::u8string_view name, uintptr_t value) = 0;

		/* set the current pc */
		virtual void setPC(env::guest_t pc) = 0;

		/* execute the next instruction */
		virtual void step() = 0;
	};
}
