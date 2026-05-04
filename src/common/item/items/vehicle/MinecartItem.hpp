#pragma once

#include "../../core/Item.hpp"
#include "../../../entity/entities/vehicle/MinecartEntity.hpp"

namespace mc {
namespace item {

/**
 * @brief 矿车物品
 *
 * 用于放置各种类型的矿车。
 *
 * 参考 MC 1.16.5 MinecartItem
 */
class MinecartItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param minecartType 矿车类型
     * @param properties 物品属性
     */
    MinecartItem(entity::AbstractMinecartEntity::Type minecartType, const ItemProperties& properties);

    ~MinecartItem() override = default;

    /**
     * @brief 在方块上使用物品
     *
     * MC 1.16.5: 在铁轨上放置矿车
     */
    [[nodiscard]] ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 获取矿车类型
     */
    [[nodiscard]] entity::AbstractMinecartEntity::Type getMinecartType() const { return m_minecartType; }

private:
    entity::AbstractMinecartEntity::Type m_minecartType;
};

} // namespace item
} // namespace mc
