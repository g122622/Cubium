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

#include "SetAttributesFunction.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/random/Random.hpp"
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

void SetAttributesFunction::addModifier(const Modifier& modifier)
{
    m_modifiers.push_back(modifier);
}

ItemStack SetAttributesFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty() || m_modifiers.empty()) {
        return stack;
    }

    math::Random& random = context.getRandom();

    // 直接获取或创建标签
    nlohmann::json& attrModifiers = stack.getOrCreateTag();

    // 确保 AttributeModifiers 是数组
    if (!attrModifiers.contains("AttributeModifiers") || !attrModifiers["AttributeModifiers"].is_array()) {
        attrModifiers["AttributeModifiers"] = nlohmann::json::array();
    }
    nlohmann::json& attrArray = attrModifiers["AttributeModifiers"];

    for (const auto& modifier : m_modifiers) {
        // 生成或使用 UUID
        std::string uuid = modifier.uuid.empty() ? _generateUUID(random) : modifier.uuid;

        // 随机选择槽位
        i32 equipmentSlot = static_cast<i32>(EquipmentSlot::MainHand);
        if (!modifier.slots.empty()) {
            size_t slotIndex = static_cast<size_t>(random.nextInt(static_cast<i32>(modifier.slots.size())));
            equipmentSlot = _parseSlotName(modifier.slots[slotIndex]);
        }

        // 生成随机值
        f64 amount = static_cast<f64>(modifier.amount.generateFloat(random));

        // 构建 AttributeModifiers 条目
        nlohmann::json attrEntry = nlohmann::json::object();
        attrEntry["AttributeName"] = modifier.attributeId;
        attrEntry["Name"] = modifier.name;
        attrEntry["Amount"] = amount;
        attrEntry["Operation"] = modifier.operation;
        attrEntry["UUID"] = uuid;
        attrEntry["Slot"] = equipmentSlot;

        // 添加到修饰符数组
        attrArray.push_back(std::move(attrEntry));
    }

    return stack;
}

std::unique_ptr<LootFunction> SetAttributesFunction::clone() const noexcept
{
    auto func = std::make_unique<SetAttributesFunction>();
    func->m_modifiers = m_modifiers;
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

i32 SetAttributesFunction::_parseSlotName(const std::string& slotName)
{
    if (slotName == "mainhand") {
        return static_cast<i32>(EquipmentSlot::MainHand);
    } else if (slotName == "offhand") {
        return static_cast<i32>(EquipmentSlot::OffHand);
    } else if (slotName == "feet") {
        return static_cast<i32>(EquipmentSlot::Feet);
    } else if (slotName == "legs") {
        return static_cast<i32>(EquipmentSlot::Legs);
    } else if (slotName == "chest") {
        return static_cast<i32>(EquipmentSlot::Chest);
    } else if (slotName == "head") {
        return static_cast<i32>(EquipmentSlot::Head);
    }
    // 默认主手
    return static_cast<i32>(EquipmentSlot::MainHand);
}

std::string SetAttributesFunction::_generateUUID(math::Random& random)
{
    const u64 part1 = (static_cast<u64>(static_cast<u32>(random.nextInt())) << 32) | static_cast<u32>(random.nextInt());
    const u64 part2 = (static_cast<u64>(static_cast<u32>(random.nextInt())) << 32) | static_cast<u32>(random.nextInt());

    // 格式化为 UUID v4 格式
    char buf[64];
    std::snprintf(buf,
        sizeof(buf),
        "%08x-%04x-%04x-%04x-%012llx",
        static_cast<u32>(part1 >> 32),                            // 8 hex digits
        static_cast<u16>((part1 >> 16) & 0xFFFF),                 // 4 hex digits
        static_cast<u16>((part1 & 0x0FFF) | 0x4000),              // 4 hex digits (version 4 UUID)
        static_cast<u16>(((part2 >> 48) & 0x3FFF) | 0x8000),      // 4 hex digits (variant 1)
        static_cast<unsigned long long>(part2 & 0xFFFFFFFFFFFF)); // 12 hex digits
    return std::string(buf);
}

} // namespace loot
} // namespace mc
