/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include "../environment.h"
#include "../../host/interface.h"

uint32_t env::detail::MappingBridge::Resolve(uint64_t address) {
	return env::Instance()->mapping().fResolve(address);
}

bool env::detail::MappingBridge::Reserve(size_t exports) {
	return (map_reserve(uint32_t(exports)) > 0);
}
uint32_t env::detail::MappingBridge::Define(const char8_t* name, size_t size, env::guest_t address) {
	return map_define(name, uint32_t(size), address);
}
void env::detail::MappingBridge::Execute(env::guest_t address) {
	return map_execute(address);
}
void env::detail::MappingBridge::Flush() {
	map_flush();
}
