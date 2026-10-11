#pragma once

#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "common/world/blockentity/processing/BeaconEntity.hpp"

namespace mc {

/** @brief 信标支付菜单，效果和层数直接来自所在方块实体。 */
class BeaconContainer final : public AbstractContainerMenu {
public:
    /** @brief 创建一格支付槽及玩家背包槽位。 */
    BeaconContainer(ContainerId id, PlayerInventory* inventory, IWorld& world, blockentity::BeaconEntity& beacon);
    [[nodiscard]] bool stillValid(const Player& player) const override;
    void removed(Player& player) override;
    [[nodiscard]] ItemStack quickMoveStack(i32 slotIndex, Player& player) override;
    /** @brief 有支付物品时更新经过白名单过滤的效果，消耗一份支付。 */
    bool updateEffects(
        std::optional<entity::effect::EffectType> primary, std::optional<entity::effect::EffectType> secondary);

private:
    blockentity::SimpleInventory m_payment;
    IWorld& m_world;
    blockentity::BeaconEntity& m_beacon;
    BlockPos m_position;
};

} // namespace mc
