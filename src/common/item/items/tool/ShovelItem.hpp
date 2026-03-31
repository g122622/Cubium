#pragma once

#include "ToolItem.hpp"
#include <unordered_map>

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
 *
 * 参考: net.minecraft.item.ShovelItem
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
    ShovelItem(const tier::IItemTier& tier,
               f32 attackDamage,
               f32 attackSpeed,
               ItemProperties properties);

    ~ShovelItem() override = default;

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
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack,
                                        const BlockState& state) const override;

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
    static std::unordered_set<const Block*> initializeEffectiveBlocks();
};

} // namespace tool
} // namespace item
} // namespace mc
