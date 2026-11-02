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
#include "common/world/block/HarvestTool.hpp"

#include <array>
#include <cstddef>
#include <unordered_map>

namespace mc {
namespace block_registry {

/**
 * @brief 单个方块的采集规则（对齐原版 `Tool.Rule` 的 minesAndDrops 分支）
 *
 * 「这个方块要用哪种工具、几档以上的工具才能挖动」在原版里并不写在方块属性上，
 * 而是由数据包标签 `#minecraft:mineable/<tool>` 与 `#minecraft:needs_*_tool` 决定。
 * 本结构就是这两类标签的解析结果。
 */
struct HarvestToolRuleSet {
    /// 有效条目数；最多 4 条（镐/斧/锹/锄），原版数据包中同一方块可同时属于多个 mineable 标签。
    u8 count = 0;

    /// 工具类型（HarvestTool 常量）→ 挖动所需的最低工具档位。
    struct Entry {
        u8 tool = 0;
        i32 level = 0;
    };

    Entry entries[4] = {};
};

/**
 * @brief 方块采集规则表（运行时从数据包标签构建）
 *
 * 原版把采集判定放在 `Tool.Rule`（方块标签 → correct_for_drops）里，方块侧只登记
 * `requiresCorrectToolForDrops`。「该方块属于哪个 mineable 标签、最低要哪一档工具」
 * 这些信息全部来自数据包，因此这里在数据包加载完成后从 `BlockTags` 实时派生，
 * 而不是编译期固化——与物品标签（`ItemTagLoader`）、区块生成数据同一口径。
 *
 * 生命周期：`BlockTagLoader::loadFromDataPackRepository()` 完成后调用 `rebuild()`，
 * 数据包重载时重新构建。
 */
class HarvestToolTable {
public:
    HarvestToolTable() = delete;

    /**
     * @brief 从当前已加载的方块标签重建采集规则表
     *
     * 读取 `mineable/{pickaxe,axe,shovel,hoe,shears}` 与
     * `needs_{stone,iron,diamond}_tool` 两组标签，递归展开 `#tag` 引用，
     * 得到「方块 → [(工具, 最低档位)]」。必须在方块标签加载完成后调用。
     */
    static void rebuild();

    /**
     * @brief 按方块 ID 查采集规则
     * @return 命中时返回规则；不需要工具的方块返回 nullptr
     */
    [[nodiscard]] static const HarvestToolRuleSet* find(const ResourceLocation& blockId);

    /**
     * @brief 清空表（数据包重载前调用）
     */
    static void clear();

    /**
     * @brief 该方块是否要求正确工具才掉落（原版 `requiresCorrectToolForDrops`）
     *
     * 与工具/档位不同，这一位**不来自数据包**：原版在方块注册时硬编码，标签只用来
     * 决定「算不算某种工具的有效方块」。因此以原版 jar 实测快照提供。
     */
    [[nodiscard]] static bool requiresCorrectToolForDrops(const ResourceLocation& blockId);

private:
    static std::unordered_map<ResourceLocation, HarvestToolRuleSet>& rules();
};

/**
 * @brief 按方块 ID 查采集规则（自由函数形式，便于工具类调用）
 * @return 命中时返回规则；不需要工具的方块返回 nullptr
 */
[[nodiscard]] const HarvestToolRuleSet* harvestToolRuleSetFor(const ResourceLocation& blockId);

/**
 * @brief 方块是否存在对应工具的采集规则
 */
[[nodiscard]] bool harvestToolRuleSetHas(const HarvestToolRuleSet& rules, u8 tool) noexcept;

/**
 * @brief 取方块在对应工具下的最低档位
 * @return 命中时返回档位；未命中返回 -1
 */
[[nodiscard]] i32 harvestToolRuleLevel(const HarvestToolRuleSet& rules, u8 tool) noexcept;

} // namespace block_registry
} // namespace mc
