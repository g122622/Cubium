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
 * IMPLIED, INCLUDING ANY PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "FireChargeItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/nether/FireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <memory>

namespace mc {
namespace item {

FireChargeItem::FireChargeItem(const ItemProperties& properties)
    : Item(properties)
    , ProjectileItem()
{}

ActionResultType FireChargeItem::onItemUse(ItemUseContext& context)
{
    Player* player = context.getPlayer();
    IWorld& world = context.getWorld();
    const BlockPos& blockPos = context.getBlockPos();
    Direction face = context.getFace();
    const BlockState* blockStatePtr = world.getBlockState(blockPos);

    if (blockStatePtr == nullptr) {
        return ActionResultType::Fail;
    }

    bool success = false;

    // 检查是否可以点燃方块（营火、蜡烛、蜡烛蛋糕等含 LIT 属性且未点燃的方块）
    if (blockStatePtr->hasProperty(BlockStateProperties::LIT())) {
        if (!blockStatePtr->get(BlockStateProperties::LIT())) {
            // 含水方块不可点燃（如含水蜡烛、含水营火）
            if (blockStatePtr->hasProperty(BlockStateProperties::WATERLOGGED()) &&
                blockStatePtr->get(BlockStateProperties::WATERLOGGED())) {
                return ActionResultType::Fail;
            }

            // 点燃方块
            BlockState newState = blockStatePtr->with(BlockStateProperties::LIT(), true);
            world.setBlockState(blockPos, &newState, 11);
            playUseSound(world, blockPos);
            success = true;
        }
    }

    // 否则尝试在点击面的相邻位置放置火焰
    if (!success) {
        BlockPos firePos = blockPos.offset(face);
        const BlockState* firePosState = world.getBlockState(firePos);

        // 只有空气位置才能放置火焰
        if (firePosState != nullptr && firePosState->isAir()) {
            // 根据下方方块选择普通火或灵魂火
            Block* fireBlock = nullptr;
            const BlockState* belowStatePtr = world.getBlockState(firePos.down());
            if (belowStatePtr != nullptr && BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(*belowStatePtr)) {
                fireBlock = VanillaBlocks::SOUL_FIRE;
            } else {
                fireBlock = VanillaBlocks::FIRE;
            }

            if (fireBlock != nullptr) {
                const BlockState& fireState = fireBlock->getDefaultState();
                IBlockReader& blockReader = static_cast<IBlockReader&>(world);
                if (fireBlock->isValidPosition(fireState, blockReader, firePos)) {
                    world.setBlockState(firePos, &fireState, 11);
                    playUseSound(world, firePos);
                    success = true;
                }
            }
        }
    }

    if (success) {
        // 消耗一个火焰弹（创造模式不消耗）
        if (player == nullptr || !player->isCreative()) {
            context.getItemStackMut().shrink(1);
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Fail;
}

void FireChargeItem::playUseSound(IWorld& world, const BlockPos& pos)
{
    Vector3 soundPos(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
    world.playSound(SoundEvents::ITEM_FIRECHARGE_USE, sound::SoundCategory::Blocks, soundPos, 1.0f, 1.0f);
}

std::unique_ptr<entity::ProjectileEntity> FireChargeItem::asProjectile(IWorld& world,
    const Vector3& position,
    const ItemStack& /*stack*/,
    f32 directionX,
    f32 directionY,
    f32 directionZ) const
{
    auto entity = entity::SmallFireballEntity::create(&world);
    if (entity) {
        entity->setPosition(position.x, position.y, position.z);
        // 火焰弹使用加速度驱动（而非速度），设置加速度方向
        auto* fireball = dynamic_cast<entity::SmallFireballEntity*>(entity.get());
        if (fireball) {
            // 加速度 = 方向 * 力度
            auto config = getDispenseConfig();
            fireball->setAcceleration(directionX * config.power, directionY * config.power, directionZ * config.power);
        }
        entity->setTypeId(entity::EntityTypeKeys::SMALL_FIREBALL);
    }
    // SmallFireballEntity 继承自 ProjectileEntity，安全的 unique_ptr 转换
    return std::unique_ptr<entity::ProjectileEntity>(static_cast<entity::ProjectileEntity*>(entity.release()));
}

ProjectileDispenseConfig FireChargeItem::getDispenseConfig() const
{
    return ProjectileDispenseConfig::fireCharge();
}

void FireChargeItem::shoot(entity::ProjectileEntity& /*projectile*/,
    f32 /*directionX*/,
    f32 /*directionY*/,
    f32 /*directionZ*/,
    f32 /*power*/,
    f32 /*uncertainty*/) const
{
    // 火焰弹在 asProjectile() 中已设置加速度，不需要 shoot()
}

} // namespace item
} // namespace mc
