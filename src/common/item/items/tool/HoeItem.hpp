#pragma once

#include "ToolItem.hpp"

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
    HoeItem(const tier::IItemTier& tier,
            i32 attackDamage,
            f32 attackSpeed,
            ItemProperties properties);

    ~HoeItem() override = default;

    /**
     * @brief 获取挖掘速度
     *
     * 对特定方块返回效率值。
     *
     * @param stack 物品堆
     * @param state 目标方块状态
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack,
                                        const BlockState& state) const override;

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
    static std::unordered_set<const Block*> initializeEffectiveBlocks();
};

} // namespace tool
} // namespace item
} // namespace mc
