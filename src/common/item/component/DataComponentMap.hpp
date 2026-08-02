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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/util/text/ITextComponent.hpp"

#include <memory>
#include <optional>
#include <variant>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc {
namespace item {
namespace component {

/**
 * @brief potion_contents 组件载荷
 *
 * 对应 Java 1.21.11 record PotionContents(Optional<Holder<Potion>> potion,
 * Optional<Integer> customColor, List<MobEffectInstance> customEffects,
 * Optional<String> customName)。
 *
 * potion：药水 holder（vanilla Potions 静态注册表 id）。本项目以资源位置字符串承载
 *   （"minecraft:night_vision"），wire codec 经 JavaPotionIdMap 转 vanilla registry id。
 * customColor：自定义颜色（ARGB），nullopt=无（用效果默认色）。
 * customEffects：自定义效果列表（独立于 potion 自带效果，叠加生效）。
 * customName：自定义药水名（显示用），nullopt=无。
 */
struct PotionContentsPayload {
    std::string potionId; ///< 药水资源位置（空串=无）
    std::optional<i32> customColor;
    std::vector<entity::effect::EffectInstance> customEffects;
    std::optional<std::string> customName;
};

/**
 * @brief 单个数据组件的异构值载荷
 *
 * variant 以 std::monostate 起始，区分"未设置"与"显式设置为某值"。
 * 各备选项对应本项目落地的 9 个组件的载荷类型。
 */
using DataComponentPayload = std::variant<std::monostate,
    i32,                                                // Damage / RepairCost
    std::unique_ptr<text::ITextComponent>,              // CustomName
    std::vector<std::unique_ptr<text::ITextComponent>>, // Lore
    item::enchant::EnchantmentContainer,                // Enchantments
    PotionContentsPayload,                              // PotionContents
    AdventureModePredicate,                             // CanPlaceOn / CanBreak
    nlohmann::json>;                                    // CustomData（对象）

/**
 * @brief 一个数据组件的 typeId + 值
 *
 * 用于 DataComponentPatch 的 added 列表。typeId 与 DataComponentType 一致，
 * 在 patch 写出/读入时作为 VarInt 前缀。
 */
struct DataComponentEntry {
    i32 typeId = 0; ///< DataComponentType 的整数 id
    DataComponentPayload value;
};

/**
 * @brief 数据组件补丁（1.21.11 DataComponentPatch）
 *
 * 表示对一个物品"基础组件（物品类型默认值）"的覆盖：added 为显式设置的新值，
 * removed 为显式移除（回到默认/不存在）。ItemStack 的 NBT 与 wire 都以此承载。
 *
 * NBT 格式（DataComponentPatch.NBT）：一个 compound，键为组件资源位置名；
 *   以 '!' 前缀的键 = removed（值为占位），其余 = added（值为该组件的 NBT）。
 * Wire 格式（STREAM_CODEC）：VarInt(addedCount) + [VarInt(typeId)+value]*
 *   + VarInt(removedCount) + [VarInt(typeId)]*。
 *
 * 本项目仅承载 9 个已落地组件；未知 typeId 在读入时跳过（透传不影响已落地字段）。
 */
class DataComponentPatch {
public:
    DataComponentPatch() = default;

    [[nodiscard]] bool isEmpty() const noexcept { return m_added.empty() && m_removed.empty(); }

    [[nodiscard]] const std::vector<DataComponentEntry>& added() const noexcept { return m_added; }
    [[nodiscard]] const std::vector<i32>& removed() const noexcept { return m_removed; }

    /// 增加一个组件值（typeId 必须在落地子集内）
    void add(i32 typeId, DataComponentPayload value) { m_added.push_back({typeId, std::move(value)}); }

    /// 增加一个组件值（枚举入口）
    void add(DataComponentType type, DataComponentPayload value)
    {
        m_added.push_back({componentTypeId(type), std::move(value)});
    }

    /// 标记移除一个组件
    void remove(i32 typeId) { m_removed.push_back(typeId); }
    void remove(DataComponentType type) { m_removed.push_back(componentTypeId(type)); }

    [[nodiscard]] std::vector<DataComponentEntry>& addedMut() noexcept { return m_added; }
    [[nodiscard]] std::vector<i32>& removedMut() noexcept { return m_removed; }

private:
    std::vector<DataComponentEntry> m_added;
    std::vector<i32> m_removed;
};

} // namespace component
} // namespace item
} // namespace mc
