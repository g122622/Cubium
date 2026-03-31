#pragma once

#include "ToolItem.hpp"
#include <unordered_set>

namespace mc {
namespace item {
namespace tool {

/**
 * @brief 镐类工具
 *
 * 有效于挖掘：
 * - 石头、矿石、铁砧等 ROCK/IRON/ANVIL 材质方块
 * - 各类矿石、石砖、熔炉等
 *
 * 挖掘等级要求：
 * - 木/金 (0): 煤矿、石头
 * - 石 (1): 铁矿、青金石矿
 * - 铁 (2): 钻石矿、金矿、红石矿
 * - 钻石 (3): 黑曜石、远古残骸（部分）
 * - 下界合金 (4): 所有可采集方块
 *
 * 参考: net.minecraft.item.PickaxeItem
 */
class PickaxeItem : public ToolItem {
public:
    /**
     * @brief 构造镐
     * @param tier 工具层级
     * @param attackDamage 基础攻击伤害（通常为 1）
     * @param attackSpeed 攻击速度修正（通常为 -2.8）
     * @param properties 物品属性
     */
    PickaxeItem(const tier::IItemTier& tier,
                i32 attackDamage,
                f32 attackSpeed,
                ItemProperties properties);

    ~PickaxeItem() override = default;

    /**
     * @brief 检查是否能采集方块
     *
     * 镐的特殊逻辑：
     * 1. 如果方块需要镐，检查挖掘等级
     * 2. 如果方块材质是 ROCK/IRON/ANVIL，总是可以采集
     *
     * @param state 目标方块状态
     * @return 如果可以采集返回 true
     */
    [[nodiscard]] bool canHarvestBlock(const BlockState& state) const override;

    /**
     * @brief 获取挖掘速度
     *
     * 对 ROCK/IRON/ANVIL 材质返回效率值。
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
     * 有效材质：ROCK, IRON, ANVIL
     *
     * @param material 材质引用
     * @return 如果材质有效返回 true
     */
    [[nodiscard]] bool isEffectiveMaterial(const Material& material) const override;

private:
    /**
     * @brief 初始化镐的有效方块集合
     * @return 有效方块集合
     *
     * 包含：石头、圆石、矿石、铁砧、熔炉等
     */
    static std::unordered_set<const Block*> initializeEffectiveBlocks();
};

} // namespace tool
} // namespace item
} // namespace mc
