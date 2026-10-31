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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace mc {

/**
 * @brief 出生条件（`spawn_conditions` 数组里的一项的 `condition` 字段）
 *
 * 对应原版 `net.minecraft.world.entity.variant.SpawnCondition` 的三个实现：
 * `BiomeCheck(HolderSet<Biome>)`、`MoonBrightnessCheck(MinMaxBounds.Doubles)`、
 * `StructureCheck(HolderSet<Structure>)`。
 *
 * 两个 `HolderSet` 类型的字段可能是**标签引用**（数据文件里写成 `#minecraft:xxx`），
 * 也可能是内联的元素列表。这里用统一的 `entries` 表示：
 * - 标签引用时 `entries` 只有一项，形如 `#minecraft:spawns_cold_variant_frogs`
 * - 内联列表时 `entries` 是各元素的 ID
 *
 * 保留 `#` 前缀是刻意的：原版 `HolderSet` 的标签与内联两种形态在语义上不等价
 * （标签会随数据包内容变化），比对时必须能区分。
 */
struct SpawnCondition {
    /// 条件类型：`biome` / `moon_brightness` / `structure`
    std::string type;
    /// 条件引用的条目（标签引用带 `#` 前缀）；`moon_brightness` 时为空
    std::vector<std::string> entries;
    /// 月相范围下限（仅 `moon_brightness`，数据文件未给出时为空）
    std::optional<double> rangeMin;
    /// 月相范围上限（仅 `moon_brightness`，数据文件未给出时为空）
    std::optional<double> rangeMax;
};

/**
 * @brief 一条出生条件选择器
 *
 * 对应原版 `PriorityProvider.Selector<SpawnContext, SpawnCondition>`，
 * 即 `(Optional<SpawnCondition> condition, int priority)`。
 *
 * `condition` 可为空——原版用空条件表示「无条件命中」，作为优先级链的兜底。
 */
struct SpawnPrioritySelector {
    std::optional<SpawnCondition> condition;
    i32 priority = 0;
};

/**
 * @brief 动物变体（`wolf_variant` / `cat_variant` / `frog_variant` /
 * `cow_variant` / `chicken_variant` / `pig_variant`）的通用数据定义
 *
 * 这六类变体的数据形状高度一致：
 * - 一个或多个贴图资源 ID
 * - 可选的 `model` 字段（牛 / 鸡 / 猪类变体有，猫 / 蛙 / 狼没有）
 * - 一组带优先级的出生条件
 *
 * 具体差异由 `AssetFields` 承载：狼有三种贴图（wild/tame/angry），其余只有
 * 一个 `asset_id`。用有序的 `(name, id)` 列表表示，既保留了字段名，又不必为
 * 六类变体各写一个类。
 */
class AnimalVariant {
public:
    /// 一项贴图资源：字段名 + 资源 ID。`asset_id` 形态时字段名为空串。
    struct AssetField {
        std::string name;
        ResourceLocation id;
    };

    /**
     * @param assets 贴图资源列表（有序，按数据文件里的字段顺序）
     * @param model 模型名（数据文件字段 `model`，没有则为空）
     * @param selectors 出生条件选择器（有序，按数组原顺序）
     */
    AnimalVariant(std::vector<AssetField> assets, std::string model, std::vector<SpawnPrioritySelector> selectors);

    [[nodiscard]] const std::vector<AssetField>& getAssets() const noexcept
    {
        return m_assets;
    }

    [[nodiscard]] const std::string& getModel() const noexcept
    {
        return m_model;
    }

    [[nodiscard]] const std::vector<SpawnPrioritySelector>& getSelectors() const noexcept
    {
        return m_selectors;
    }

private:
    std::vector<AssetField> m_assets;
    std::string m_model;
    std::vector<SpawnPrioritySelector> m_selectors;
};

} // namespace mc
