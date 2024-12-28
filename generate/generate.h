#pragma once

#include "gen-common.h"
#include "generator/generator.h"
#include "core/gen-core.h"
#include "block/gen-block.h"
#include "glue/gen-glue.h"

namespace gen {
	gen::Generator* Instance();
	bool SetInstance(std::unique_ptr<gen::Translator>&& translator, uint32_t translationDepth, bool singleStep);
	void ClearInstance();
}
