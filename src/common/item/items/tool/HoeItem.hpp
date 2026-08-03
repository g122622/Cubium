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
 * @brief 锄类工具
 *
 * 有效于挖掘：
 * - 干草块 (HAY_BLOCK)
 * - 海绵 (SPONGE)
 * - 树叶 (LEAVES)
 * - 苔藓 (MOSS)
 * - 地狱疣块 (NETHER_WART_BLOCK)
 *
 * 特殊功能：
 * - 右键泥土/草地可创建耕地（dirt/grass_block -> farmland）
 *
 * 攻击伤害：最低
 * 攻击速度：比其他工具慢（约 -1.0）
 *
 * 参考: net.minecraft.item.HoeItem
 */
class HoeItem : public ToolItem {
public:
    /**
     * @brief 构造锄
     * @param tier 工具层级
     * @param attackDamage 基础攻击伤害（通常为 0）
     * @param attackSpeed 攻击速度修正（通常为 -2.0）
     * @param properties 物品属性
     */
    HoeItem(const tier::IItemTier& tier, i32 attackDamage, f32 attackSpeed, ItemProperties properties);

    ~HoeItem() override = default;

    /**
     * @brief 方块交互（耕地创建）
     *
     * 右键泥土/草地可创建耕地：
     * - 播放音效
     * - 消耗耐久
     * - 转换方块
     *
     * @param context 使用上下文
     * @return 动作结果类型
     */
    [[nodiscard]] ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 获取挖掘速度
     *
     * 对特定方块返回效率值。
     *
     * @param stack 物品堆
     * @param state 目标方块状态
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack, const BlockState& state) const override;

    /**
     * @brief 获取耕地转换后的方块
     * @param original 原始方块
     * @return 转换后的方块，如果不可转换则返回 nullptr
     */
    [[nodiscard]] static const Block* getTilledBlock(const Block* original) noexcept;

protected:
    /**
     * @brief 检查材质是否有效
     *
     * 锄主要对特定方块有效，而非材质。
     *
     * @param material 材质引用
     * @return 如果材质有效返回 true
     */
    [[nodiscard]] bool isEffectiveMaterial(const Material& material) const override;

private:
    /**
     * @brief 初始化锄的有效方块集合
     * @return 有效方块集合
     */
    static std::unordered_set<const Block*> _initializeEffectiveBlocks();

    /**
     * @brief 获取耕地映射表（延迟初始化）
     *
     * 使用"construct on first use"模式，确保静态方法调用前映射表已初始化
     *
     * @return 泥土/草地 -> 耕地 映射的引用
     */
    static std::unordered_map<const Block*, const Block*>& _getTillingMap();
};

} // namespace tool
} // namespace item
} // namespace mc
