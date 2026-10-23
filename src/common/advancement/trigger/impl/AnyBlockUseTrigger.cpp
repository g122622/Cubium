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

#include "AnyBlockUseTrigger.hpp"

#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// ========== AnyBlockUseTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> AnyBlockUseTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<AnyBlockUseTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

// ========== AnyBlockUseTriggerInstance ==========

Result<void> AnyBlockUseTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }
    // item_used_on_block 的条件：{"location": [{"condition": "minecraft:location_check", ...}]}
    if (json.contains("location")) {
        auto result = ContextAwarePredicate::fromJson(json["location"]);
        if (result.failed()) {
            return result.error();
        }
        m_locationContext = std::move(result.value());
    }
    return {};
}

bool AnyBlockUseTriggerInstance::test(const CriterionContext& context) const
{
    return m_locationContext.test(context);
}

nlohmann::json AnyBlockUseTriggerInstance::conditionsToJson() const
{
    if (m_locationContext.isAny()) {
        return nullptr; // 无条件（default_block_use）
    }
    return nlohmann::json{{"location", m_locationContext.toJson()}};
}

// ========== ItemUsedOnBlockTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> ItemUsedOnBlockTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<AnyBlockUseTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

} // namespace mc::advancement
