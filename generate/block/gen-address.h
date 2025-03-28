/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"
#include "../mapping/mapping-writer.h"

namespace gen::detail {
	struct OpenAddress {
		wasm::Function function;
		env::guest_t address = 0;
	};
	struct PlaceAddress {
		wasm::Function function;
		uint32_t index = 0;
		bool thisModule = false;
		bool alreadyExists = false;
	};

	class Addresses {
	private:
		struct Placement {
			wasm::Function function;
			uint32_t index = 0;
			bool thisModule = false;
			bool alreadyExists = false;
			bool incomplete = false;
		};
		struct Queued {
		public:
			env::guest_t address = 0;
			size_t depth = 0;

		public:
			friend bool operator<(const Queued& l, const Queued& r) {
				return (l.depth < r.depth);
			}
		};

	private:
		std::unordered_map<env::guest_t, Placement> pTranslated;
		std::unordered_map<env::guest_t, uint32_t> pLinks;
		std::priority_queue<Queued> pQueue;
		wasm::Table pAddresses;
		wasm::Prototype pBlockPrototype;
		size_t pDepth = 0;
		bool pNeedsStartup = false;

	public:
		Addresses() = default;

	private:
		Placement& fPush(env::guest_t address, size_t depth);

	public:
		detail::PlaceAddress pushLocal(env::guest_t address);
		void pushRoot(env::guest_t address);

	public:
		void setup(const wasm::Prototype& blockPrototype);
		const wasm::Table& addresses();
		bool empty() const;
		detail::OpenAddress start();
		std::vector<env::BlockExport> close(const detail::MappingState& mappingState);
	};
}
