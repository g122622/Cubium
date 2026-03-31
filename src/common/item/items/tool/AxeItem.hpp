#pragma once

#include "ToolItem.hpp"
#include <unordered_map>

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
 *
 * 参考: net.minecraft.item.AxeItem
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
    AxeItem(const tier::IItemTier& tier,
            f32 attackDamage,
            f32 attackSpeed,
            ItemProperties properties);

    ~AxeItem() override = default;

    /**
     * @brief 获取挖掘速度
     *
     * 对 WOOD, NETHER_WOOD, PLANT, GOURD, BAMBOO 材质返回效率值。
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
    static std::unordered_set<const Block*> initializeEffectiveBlocks();

    // 原木 -> 去皮原木 的映射（未来实现去皮功能）
    // static std::unordered_map<const Block*, const Block*> s_strippingMap;
};

} // namespace tool
} // namespace item
} // namespace mc
