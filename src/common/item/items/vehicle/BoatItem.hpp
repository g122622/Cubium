/**
 * @file BoatItem.hpp
 * @brief 船物品类 - 在水面上放置船实体
 *
 * MC 1.16.5 参考: net.minecraft.item.BoatItem
 *
 * 船物品是一种特殊的物品，当玩家右键点击水面或地面时，
 * 会在该位置生成一个船实体。船有 6 种木材类型。
 */

#pragma once

#include "../../../entity/entities/vehicle/BoatEntity.hpp"
#include "../../context/ItemUseContext.hpp"
#include "../../core/Item.hpp"

namespace mc {
namespace item {

/**
 * @brief 船物品类
 *
 * 继承自 Item 基类，实现放置船实体的功能。
 *
 * 使用方式：
 * - 玩家右键点击水面或陆地
 * - 在击中位置生成对应类型的船实体
 * - 船自动朝向玩家的朝向
 * - 非创造模式消耗物品
 */
class BoatItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param boatType 船的木材类型
     * @param properties 物品属性
     */
    BoatItem(entity::BoatEntity::Type boatType, const ItemProperties& properties);

    ~BoatItem() override = default;

    /**
     * @brief 获取船的类型
     * @return 船的木材类型
     */
    [[nodiscard]] entity::BoatEntity::Type getBoatType() const { return m_boatType; }

    /**
     * @brief 在方块上使用物品
     *
     * MC 1.16.5 行为:
     * 1. 获取击中点位置
     * 2. 在击中位置生成船实体
     * 3. 设置船的朝向为玩家的朝向
     * 4. 检查碰撞，如果有碰撞返回 Fail
     * 5. 服务端生成船实体
     * 6. 非创造模式消耗物品
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    [[nodiscard]] ActionResultType onItemUse(ItemUseContext& context) override;

private:
    entity::BoatEntity::Type m_boatType;
};

} // namespace item
} // namespace mc
