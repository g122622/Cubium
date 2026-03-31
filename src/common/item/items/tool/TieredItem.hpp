#pragma once

#include "../../core/Item.hpp"
#include "../../tier/IItemTier.hpp"

namespace mc {
namespace item {
namespace tool {

/**
 * @brief 层级物品基类
 *
 * 所有具有材质层级的物品（工具、剑、护甲）的基类。
 * 提供层级相关的基础属性：耐久度、附魔能力、修复材料。
 *
 * 继承关系：
 * - TieredItem → ToolItem → PickaxeItem/AxeItem/ShovelItem/HoeItem
 * - TieredItem → SwordItem
 *
 * 参考: net.minecraft.item.TieredItem
 */
class TieredItem : public Item {
public:
    /**
     * @brief 构造层级物品
     * @param tier 工具层级（木、石、铁、金、钻石、下界合金）
     * @param properties 物品属性（耐久度会从层级自动设置）
     */
    TieredItem(const tier::IItemTier& tier, ItemProperties properties);

    ~TieredItem() override = default;

    /**
     * @brief 获取工具层级
     * @return 层级引用
     */
    [[nodiscard]] const tier::IItemTier& getTier() const { return m_tier; }

    /**
     * @brief 获取物品附魔能力
     *
     * 返回层级的附魔能力值。
     * 金制品最高(22)，钻石最低(10)。
     *
     * @return 附魔能力值
     */
    [[nodiscard]] i32 getItemEnchantability() const override {
        return m_tier.getEnchantability();
    }

    /**
     * @brief 检查是否可以用指定材料修复
     *
     * 检查修复材料是否匹配层级的修复材料。
     * 用于铁砧修复机制。
     *
     * @param toRepair 待修复的物品堆
     * @param repair 修复材料物品堆
     * @return 如果可以修复返回 true
     */
    [[nodiscard]] bool isRepairable(const ItemStack& toRepair,
                                     const ItemStack& repair) const;

protected:
    const tier::IItemTier& m_tier;
};

} // namespace tool
} // namespace item
} // namespace mc
