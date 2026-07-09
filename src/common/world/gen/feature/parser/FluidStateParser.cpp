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

#include "FluidStateParser.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/fluid/Fluid.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <string>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {
namespace FluidStateParser {

Result<const fluid::FluidState*> parse(const nlohmann::json& stateObj)
{
    if (!stateObj.is_object() || !stateObj.contains("Name") || !stateObj["Name"].is_string()) {
        return Error(ErrorCode::InvalidData, "fluid state JSON missing 'Name' string field");
    }

    const std::string fluidName = stateObj["Name"].get<std::string>();
    const ResourceLocation fluidLoc(fluidName);
    fluid::Fluid* fluid = fluid::Fluid::getFluid(fluidLoc);
    if (fluid == nullptr) {
        return Error(ErrorCode::NotFound, "unknown fluid '" + fluidName + "' in fluid state JSON");
    }

    const fluid::FluidState* currentState = &fluid->defaultState();

    if (!stateObj.contains("Properties") || !stateObj["Properties"].is_object()) {
        return currentState;
    }

    for (const auto& [propName, propValue] : stateObj["Properties"].items()) {
        if (!propValue.is_string()) {
            continue;
        }
        const IProperty* prop = fluid->stateContainer().getProperty(propName);
        if (prop == nullptr) {
            spdlog::warn("FluidStateParser: unknown property '{}' on fluid '{}'", propName, fluidName);
            continue;
        }
        auto valueIndex = prop->parseValue(propValue.get<std::string>());
        if (!valueIndex.has_value()) {
            spdlog::warn("FluidStateParser: invalid value '{}' for property '{}' on fluid '{}'",
                propValue.get<std::string>(),
                propName,
                fluidName);
            continue;
        }
        currentState = &currentState->withValueIndex(*prop, *valueIndex);
    }

    return currentState;
}

} // namespace FluidStateParser
} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
