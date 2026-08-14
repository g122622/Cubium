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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY,WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SpawnEggItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <utility>

namespace mc {
namespace item {

SpawnEggItem::SpawnEggItem(
    entity::EntityType entityType, u32 primaryColor, u32 secondaryColor, const ItemProperties& properties)
    : Item(properties)
    , m_entityType(std::move(entityType))
    , m_primaryColor(primaryColor)
    , m_secondaryColor(secondaryColor)
{}

ActionResultType SpawnEggItem::onItemUse(ItemUseContext& context)
{
    IWorld& world = context.getWorld();
    BlockPos pos = context.getBlockPos();

    // 客户端直接预测成功
    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 如果点击的方块是刷怪笼，设置刷怪笼的实体类型而非生成生物
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::MobSpawner) {
        auto* spawner = static_cast<blockentity::MobSpawnerBlockEntity*>(blockEntity);
        ResourceLocation entityId(m_entityType.name());
        math::Random& rng = world.getRandom();
        spawner->setEntityId(entityId, rng);

        // 非创造模式下消耗刷怪蛋
        Player* player = context.getPlayer();
        if (player && !player->isCreative()) {
            context.getItemStackMut().shrink(1);
        }
        return ActionResultType::Success;
    }

    // 常规路径：在方块面上方生成实体
    Direction face = context.getFace();
    BlockPos spawnPos = pos.offset(face);

    // 检查位置是否可替换
    const BlockState* state = world.getBlockState(spawnPos);
    if (state != nullptr && !state->canBeReplaced()) {
        return ActionResultType::Fail;
    }

    // 生成实体
    if (spawnEntity(world, spawnPos, world::spawn::SpawnReason::SpawnEgg)) {
        // 消耗物品 (非创造模式)
        Player* player = context.getPlayer();
        if (player && !player->isCreative()) {
            context.getItemStackMut().shrink(1);
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Fail;
}

ItemActionResult SpawnEggItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    // 在玩家位置生成实体
    if (world.isClientSide()) {
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    Vector3 pos = player.position();
    BlockPos spawnPos(static_cast<i32>(pos.x), static_cast<i32>(pos.y), static_cast<i32>(pos.z));

    if (spawnEntity(world, spawnPos, world::spawn::SpawnReason::SpawnEgg)) {
        if (!player.isCreative()) {
            player.getHeldItem(hand).shrink(1);
        }
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    return ItemActionResult::pass(player.getHeldItem(hand));
}

bool SpawnEggItem::spawnEntity(IWorld& world, const BlockPos& pos, world::spawn::SpawnReason spawnReason) const
{
    // 通过实体注册表创建实体
    // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        return false;
    }
    auto entity = m_entityType.create(&world, *registry);
    if (!entity) {
        return false;
    }

    // 设置位置 (方块中心上方)
    f32 x = static_cast<f32>(pos.x) + 0.5f;
    f32 y = static_cast<f32>(pos.y);
    f32 z = static_cast<f32>(pos.z) + 0.5f;
    entity->setPosition(x, y, z);

    // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化（使用位置感知的区域难度）
    auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
    if (mobEntity != nullptr) {
        entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(world, pos);
        mobEntity->finalizeSpawn(world, difficultyInstance, spawnReason);
    }

    // 生成实体
    world.spawnEntity(std::move(entity));
    return true;
}

} // namespace item
} // namespace mc
