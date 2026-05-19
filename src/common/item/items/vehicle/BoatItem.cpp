/**
 * @file BoatItem.cpp
 * @brief 船物品类实现
 *
 * MC 1.16.5 参考: net.minecraft.item.BoatItem
 */

#include "BoatItem.hpp"

#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/vehicle/BoatEntity.hpp"
#include "../../../util/AxisAlignedBB.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../context/ItemUseContext.hpp"
#include "../../core/ItemStack.hpp"

namespace mc {
namespace item {

BoatItem::BoatItem(entity::BoatEntity::Type boatType, const ItemProperties& properties)
    : Item(std::move(properties))
    , m_boatType(boatType)
{}

ActionResultType BoatItem::onItemUse(ItemUseContext& context)
{
    // MC 1.16.5: BoatItem.onItemRightClick
    // 船物品使用右键点击放置，而非左键点击方块

    IWorld* world = &context.world();
    if (world == nullptr) {
        return ActionResultType::Fail;
    }

    // 服务端才能创建实体
    if (world->isClientSide()) {
        return ActionResultType::Success;
    }

    Player* player = context.player();
    if (player == nullptr) {
        return ActionResultType::Fail;
    }

    // MC 1.16.5: 计算船的生成位置
    // 船生成在击中点位置
    const Vector3& hitPos = context.hitPosition();
    f32 x = hitPos.x;
    f32 y = hitPos.y;
    f32 z = hitPos.z;

    // MC 1.16.5: 创建船实体
    auto boat = std::make_unique<entity::BoatEntity>(m_boatType);
    boat->setPosition(x, y, z);

    // MC 1.16.5: 设置船的朝向为玩家的朝向
    boat->setRotation(context.getPlayerYaw());

    // MC 1.16.5: 检查碰撞
    // 如果船的碰撞箱（缩小0.1格）与其他实体碰撞，返回 Fail
    AxisAlignedBB boatBox = boat->boundingBox().grow(-0.1f);
    if (!world->hasNoCollisions(boatBox)) {
        return ActionResultType::Fail;
    }

    // MC 1.16.5: 生成船实体
    world->spawnEntity(std::move(boat));

    // MC 1.16.5: 非创造模式消耗物品
    ItemStack& stack = context.getItemStackMut();
    if (!player->isCreative()) {
        stack.shrink(1);
    }

    return ActionResultType::Success;
}

} // namespace item
} // namespace mc
