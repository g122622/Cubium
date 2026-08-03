/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "SetStewEffectFunction.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

void SetStewEffectFunction::addEffect(const std::string& effectId, const RandomValueRange& duration)
{
    m_effects.push_back({effectId, duration});
}

ItemStack SetStewEffectFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty() || m_effects.empty()) {
        return stack;
    }

    // 只对谜之炖菜生效
    if (stack.getItem() != Items::SUSPICIOUS_STEW) {
        return stack;
    }

    // 随机选择一个效果
    math::Random& random = context.getRandom();
    const size_t effectIndex = static_cast<size_t>(random.nextInt(static_cast<i32>(m_effects.size())));
    const EffectEntry& entry = m_effects[effectIndex];

    // 解析效果类型
    // 支持两种格式：
    // 1. 资源位置格式 "minecraft:poison"
    // 2. 简写格式 "poison"
    std::optional<entity::effect::EffectType> effectType;
    if (entry.effectId.find(':') != std::string::npos) {
        // 完整资源位置格式
        effectType = entity::effect::getEffectByResourceLocation(ResourceLocation(entry.effectId));
    } else {
        // 简写格式，默认使用 minecraft 命名空间
        effectType = entity::effect::getEffectByResourceLocation(ResourceLocation("minecraft", entry.effectId));
    }

    if (!effectType.has_value()) {
        // 无效的效果ID，返回原物品
        return stack;
    }

    // 生成持续时间（秒）
    // 非瞬间效果需要乘以 20 转换为 tick
    i32 durationTicks = entry.duration.generateInt(random);
    if (!entity::effect::isInstantEffect(effectType.value())) {
        // 非瞬间效果：秒转tick
        durationTicks *= 20;
    }

    // 写入物品的 NBT 数据
    // 格式: {Effects: [{EffectId: byte, EffectDuration: int}, ...]}
    nlohmann::json& tag = stack.getOrCreateTag();

    // 获取或创建 Effects 数组（使用引用）
    if (!tag.contains("Effects") || !tag["Effects"].is_array()) {
        tag["Effects"] = nlohmann::json::array();
    }
    nlohmann::json& effectsArray = tag["Effects"];

    // 添加新效果
    nlohmann::json effectJson;
    effectJson["EffectId"] = static_cast<i8>(static_cast<i32>(effectType.value()));
    effectJson["EffectDuration"] = durationTicks;
    effectsArray.push_back(std::move(effectJson));

    // Effects 已经直接在 tag 中更新了，不需要再次赋值

    return stack;
}

std::unique_ptr<LootFunction> SetStewEffectFunction::clone() const noexcept
{
    auto func = std::make_unique<SetStewEffectFunction>();
    func->m_effects = m_effects;
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
