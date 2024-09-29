#include "env-common.h"

env::Environment::Environment(std::u8string_view name) : pName{ name } {
	str::FormatTo(pLogHeader, u8"Env [{:>12}]: ", pName);
}

const std::u8string& env::Environment::name() const {
	return pName;
}
env::environment_t env::Environment::identifier() const {
	return pIdentifier;
}
