/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BlockStateParser.hpp"

#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"

#include <spdlog/spdlog.h>

#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {
namespace BlockStateParser {

Result<const BlockState*> parse(const nlohmann::json& stateObj)
{
    if (!stateObj.is_object() || !stateObj.contains("Name") || !stateObj["Name"].is_string()) {
        return Error(ErrorCode::InvalidData, "block state JSON missing 'Name' string field");
    }

    const std::string blockName = stateObj["Name"].get<std::string>();
    const ResourceLocation blockLoc(blockName);
    const Block* block = BlockRegistry::instance().getBlock(blockLoc);
    if (block == nullptr) {
        return Error(ErrorCode::NotFound, "unknown block '" + blockName + "' in block state JSON");
    }

    const BlockState* currentState = &block->defaultState();

    if (!stateObj.contains("Properties") || !stateObj["Properties"].is_object()) {
        return currentState;
    }

    for (const auto& [propName, propValue] : stateObj["Properties"].items()) {
        if (!propValue.is_string()) {
            continue;
        }
        const IProperty* prop = block->stateContainer().getProperty(propName);
        if (prop == nullptr) {
            spdlog::warn("BlockStateParser: unknown property '{}' on block '{}'", propName, blockName);
            continue;
        }
        auto valueIndex = prop->parseValue(propValue.get<std::string>());
        if (!valueIndex.has_value()) {
            spdlog::warn("BlockStateParser: invalid value '{}' for property '{}' on block '{}'",
                propValue.get<std::string>(),
                propName,
                blockName);
            continue;
        }
        currentState = &currentState->withValueIndex(*prop, *valueIndex);
    }

    return currentState;
}

} // namespace BlockStateParser
} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
