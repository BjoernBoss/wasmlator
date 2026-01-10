/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include "host-interact.h"
#include "../host/interface.h"

extern "C" void emscripten_notify_memory_growth(uint32_t) {
	host_check_metrics();
}

uint32_t host::GetRandom() {
	return host_random();
}

uint64_t host::GetStampUS() {
	return host_time_us();
}

int32_t host::GetTimeZoneMIN() {
	return host_timezone_min();
}

void host::HostCheckMetrics() {
	host_check_metrics();
}

void host::PrintOut(std::u8string_view msg) {
	std::u8string actual = str::u8::Build(u8"O:", msg);
	host_message(actual.data(), uint32_t(actual.size()));
}

void host::PrintOutLn(std::u8string_view msg) {
	std::u8string actual = str::u8::Build(u8"O:", msg, u8'\n');
	host_message(actual.data(), uint32_t(actual.size()));
}
