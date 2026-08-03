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

#include "ChanneledLightningTrigger.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

// 注意：trigger() 方法的完整实现需要服务端模块的支持
// 服务端代码应包含 server/advancement/TriggerInstantiation.hpp
// 并使用 triggerWithPredicate() 方法或直接调用基类的 trigger() 模板方法

namespace mc::advancement {

// ========== ChanneledLightningTriggerInstance ==========

ChanneledLightningTriggerInstance::ChanneledLightningTriggerInstance(std::vector<EntityPredicate> victims)
    : m_victims(std::move(victims))
{}

bool ChanneledLightningTriggerInstance::test(const std::vector<const Entity*>& victims) const
{
    // 检查所有谓词是否都能匹配到至少一个实体
    // 如果没有谓词，则匹配任意情况
    if (m_victims.empty()) {
        return true;
    }

    // 对于每个谓词，检查是否至少有一个受害者实体匹配
    for (const auto& predicate : m_victims) {
        bool matched = false;
        for (const Entity* victim : victims) {
            if (victim != nullptr && predicate.test(*victim)) {
                matched = true;
                break;
            }
        }
        // 如果这个谓词没有任何匹配的实体，则整体不匹配
        if (!matched) {
            return false;
        }
    }

    return true;
}

Result<void> ChanneledLightningTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    // JSON 格式:
    // {
    //   "victims": [
    //     { "type": "minecraft:zombie" },
    //     { "type": "minecraft:skeleton" }
    //   ]
    // }
    if (json.contains("victims")) {
        const auto& victimsArray = json["victims"];
        if (!victimsArray.is_array()) {
            return Error(ErrorCode::InvalidData, "ChanneledLightningTrigger: 'victims' must be an array");
        }

        m_victims.clear();
        m_victims.reserve(victimsArray.size());

        for (const auto& victimJson : victimsArray) {
            auto result = EntityPredicate::fromJson(victimJson);
            if (result.failed()) {
                return result.error();
            }
            m_victims.push_back(result.value());
        }
    }

    return {};
}

nlohmann::json ChanneledLightningTriggerInstance::conditionsToJson() const
{
    if (m_victims.empty()) {
        return nullptr;
    }

    nlohmann::json json;
    nlohmann::json victimsArray = nlohmann::json::array();

    for (const auto& predicate : m_victims) {
        victimsArray.push_back(predicate.toJson());
    }

    json["victims"] = std::move(victimsArray);
    return json;
}

// ========== ChanneledLightningTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> ChanneledLightningTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<ChanneledLightningTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void ChanneledLightningTrigger::trigger(ServerPlayer& player, const std::vector<const Entity*>& victims)
{
    // 此方法在 common 模块中无法完整实现，因为需要访问 PlayerAdvancements 的完整定义
    // 服务端代码应使用以下方式触发检测：
    //
    // 方法：使用 TriggerInstantiation.hpp 中的 trigger 模板方法
    // #include "server/advancement/TriggerInstantiation.hpp"
    // auto* trigger = CriterionTriggers::instance().getTrigger<ChanneledLightningTrigger>();
    // trigger->AbstractCriterionTrigger<ChanneledLightningTriggerInstance>::trigger(
    //     *player.getAdvancements(),
    //     [&victims](const ChanneledLightningTriggerInstance& instance) {
    //         return instance.test(victims);
    //     }
    // );
    //
    // 参考：server/advancement/AdvancementEventHandler.hpp
    MC_UNUSED(player);
    MC_UNUSED(victims);
}

} // namespace mc::advancement
