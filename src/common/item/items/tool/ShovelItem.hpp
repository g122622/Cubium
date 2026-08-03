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
 * @brief 锹类工具
 *
 * 有效于挖掘：
 * - 泥土材质 (EARTH)
 * - 沙子材质 (SAND)
 * - 雪材质 (SNOW)
 * - 粘土 (CLAY)
 *
 * 特殊功能：
 * - 右键草地可创建土径（grass_block -> grass_path）
 *
 * 攻击伤害：比其他工具低
 * 攻击速度：正常（约 -2.9）
 */
class ShovelItem : public ToolItem {
public:
    /**
     * @brief 构造锹
     * @param tier 工具层级
     * @param attackDamage 基础攻击伤害（通常为 1.5 - tier.getAttackDamage()）
     * @param attackSpeed 攻击速度修正（通常为 -2.9）
     * @param properties 物品属性
     */
    ShovelItem(const tier::IItemTier& tier, f32 attackDamage, f32 attackSpeed, ItemProperties properties);

    ~ShovelItem() override = default;

    /**
     * @brief 方块交互（土径创建）
     *
     * 右键草地可创建土径：
     * - 播放音效
     * - 消耗耐久
     * - 转换方块
     *
     * @param context 使用上下文
     * @return 动作结果类型
     */
    [[nodiscard]] ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 检查是否能采集方块
     *
     * 锹对雪类方块有特殊处理。
     *
     * @param state 目标方块状态
     * @return 如果可以采集返回 true
     */
    [[nodiscard]] bool canHarvestBlock(const BlockState& state) const override;

    /**
     * @brief 获取挖掘速度
     *
     * 对 EARTH, SAND, SNOW 材质返回效率值。
     *
     * @param stack 物品堆
     * @param state 目标方块状态
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack, const BlockState& state) const override;

    /**
     * @brief 获取土径转换后的方块
     * @param original 原始方块
     * @return 转换后的方块，如果不可转换则返回 nullptr
     */
    [[nodiscard]] static const Block* getPathBlock(const Block* original);

protected:
    /**
     * @brief 检查材质是否有效
     *
     * 有效材质：EARTH, SAND, SNOW
     *
     * @param material 材质引用
     * @return 如果材质有效返回 true
     */
    [[nodiscard]] bool isEffectiveMaterial(const Material& material) const override;

private:
    /**
     * @brief 初始化锹的有效方块集合
     * @return 有效方块集合
     */
    static std::unordered_set<const Block*> _initializeEffectiveBlocks();

    /**
     * @brief 获取土径映射表（延迟初始化）
     *
     * 使用"construct on first use"模式，确保静态方法调用前映射表已初始化
     *
     * @return 草方块 -> 土径 映射的引用
     */
    static std::unordered_map<const Block*, const Block*>& _getPathMap();
};

} // namespace tool
} // namespace item
} // namespace mc
