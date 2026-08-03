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

#include "ToolItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/tier/IItemTier.hpp"
#include "common/world/block/Material.hpp"
#include <unordered_map>
#include <unordered_set>

namespace mc {
namespace item {
namespace tool {

/**
 * @brief 斧类工具
 *
 * 有效于挖掘：
 * - 木头材质 (WOOD, NETHER_WOOD)
 * - 植物材质 (PLANT, TALL_PLANTS)
 * - 葫芦材质 (GOURD - 南瓜、西瓜)
 * - 竹子 (BAMBOO)
 *
 * 特殊功能：
 * - 右键原木可去皮（log -> stripped_log）
 *
 * 攻击伤害：比同层级其他工具高
 * 攻击速度：比其他工具慢（约 -3.0）
 */
class AxeItem : public ToolItem {
public:
    /**
     * @brief 构造斧
     * @param tier 工具层级
     * @param attackDamage 基础攻击伤害（通常为 6.0 - tier.getAttackDamage()）
     * @param attackSpeed 攻击速度修正（通常为 -3.0）
     * @param properties 物品属性
     */
    AxeItem(const tier::IItemTier& tier, f32 attackDamage, f32 attackSpeed, ItemProperties properties);

    ~AxeItem() override = default;

    /**
     * @brief 方块交互
     *
     * 右键交互按以下优先级依次检查：
     * 1. 去皮（原木 → 去皮原木）：播放 ITEM_AXE_STRIP 音效
     * 2. 去氧化/刮削（氧化铜 → 上一级氧化）：播放 SCRAPE 粒子效果
     * 3. 除蜡（涂蜡铜 → 未涂蜡铜）：播放 WAX_OFF 粒子效果
     *
     * 每个步骤独立检查，只有第一个匹配的步骤被执行。
     *
     * @param context 使用上下文
     * @return 动作结果类型
     */
    [[nodiscard]] ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 获取挖掘速度
     *
     * 对 WOOD, NETHER_WOOD, PLANT, GOURD, BAMBOO 材质返回效率值。
     *
     * @param stack 物品堆
     * @param state 目标方块状态
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack, const BlockState& state) const override;

    /**
     * @brief 获取去皮后的方块
     * @param original 原始方块
     * @return 去皮后的方块，如果不可去皮则返回 nullptr
     */
    [[nodiscard]] static const Block* getStrippedBlock(const Block* original);

protected:
    /**
     * @brief 检查材质是否有效
     *
     * 有效材质：WOOD, NETHER_WOOD, PLANT, GOURD, BAMBOO
     *
     * @param material 材质引用
     * @return 如果材质有效返回 true
     */
    [[nodiscard]] bool isEffectiveMaterial(const Material& material) const override;

private:
    /**
     * @brief 初始化斧的有效方块集合
     * @return 有效方块集合
     */
    static std::unordered_set<const Block*> _initializeEffectiveBlocks();

    /**
     * @brief 获取去皮映射表（延迟初始化）
     *
     * 使用"construct on first use"模式，确保静态方法调用前映射表已初始化
     *
     * @return 原木 -> 去皮原木 映射的引用
     */
    static std::unordered_map<const Block*, const Block*>& _getStrippingMap();
};

} // namespace tool
} // namespace item
} // namespace mc
