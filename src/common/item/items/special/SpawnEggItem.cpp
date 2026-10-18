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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
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
#include "common/util/math/ray/Ray.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <optional>
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

    // 取点击方块的状态（用于空碰撞形状判断与 gameEvent 上下文）
    const BlockState* clickedState = world.getBlockState(pos);

    // 如果点击的方块是刷怪笼，设置刷怪笼的实体类型而非生成生物
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::MobSpawner) {
        // 刷怪笼被 spawnerBlocksWork 游戏规则禁用时不接受刷怪蛋设置实体类型（对齐 vanilla
        // SpawnEggItem.useOn → ServerLevel.isSpawnerBlockEnabled = getGameRules().SPAWNER_BLOCKS_WORK）。
        if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::SPAWNER_BLOCKS_WORK)) {
            Player* player = context.getPlayer();
            if (player != nullptr) {
                player->sendStatusMessage("Spawner blocks are disabled", true);
            }
            return ActionResultType::Fail;
        }

        auto* spawner = static_cast<blockentity::MobSpawnerBlockEntity*>(blockEntity);
        ResourceLocation entityId(m_entityType.name());
        math::IRandom& rng = world.getRandom();
        spawner->setEntityId(entityId, rng);

        // 通知客户端方块变更并触发 BLOCK_CHANGE 振动事件
        world.notifyBlockUpdate(pos);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, clickedState);

        // 非创造模式下消耗刷怪蛋
        Player* player = context.getPlayer();
        if (player && !player->isCreative()) {
            context.getItemStackMut().shrink(1);
        }
        return ActionResultType::Success;
    }

    // 常规路径：确定生成位置
    // 对齐 Java useOn：若点击方块碰撞形状为空（草、花、火把等），在方块自身位置生成；否则在面偏移位置生成。
    // Java spawnMob 不检查生成位置的可替换性（实体可与方块共处，如猪站在火把格），此处保持一致。
    Direction face = context.getFace();
    BlockPos spawnPos = pos;
    if (clickedState != nullptr && !clickedState->getCollisionShape().isEmpty()) {
        spawnPos = pos.offset(face);
    }

    // 生成实体
    if (spawnEntity(world, spawnPos, world::spawn::SpawnReason::SpawnEgg)) {
        // 触发 ENTITY_PLACE 振动事件
        world.gameEvent(gameevent::GameEvents::ENTITY_PLACE, spawnPos, clickedState);

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
    // 客户端直接预测成功
    if (world.isClientSide()) {
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    // 沿视线做仅取流体源的射线检测（对应 Java getPlayerPOVHitResult(level, player, Fluid.SOURCE_ONLY)），
    // 命中首个水源方块则在该位置生成实体（用于在水里放鱿鱼等）。射程取玩家方块交互距离属性。
    const Vector3 eyePosition = player.getEyePosition();
    const Ray ray = Ray::fromAngles(eyePosition, player.pitch(), player.yaw());
    const BlockRaycastResult fluidHit =
        raycastFluidSource(RaycastContext(ray, static_cast<f32>(player.blockInteractionRange())), world);

    // 未命中方块：不处理（Java 返回 PASS）
    if (!fluidHit.isHit()) {
        return ItemActionResult::pass(player.getHeldItem(hand));
    }

    // 权限门控（Java use）：世界级交互权限与玩家物品使用权限任一失败即整体失败、不消耗物品。
    const BlockPos hitPos = fluidHit.blockPos();
    if (!world.mayInteract(player, hitPos) ||
        !player.mayUseItemAt(world, hitPos, fluidHit.face(), player.getHeldItem(hand))) {
        return ItemActionResult::fail(player.getHeldItem(hand));
    }

    // 与 Java 一致：命中非液体方块上的流体源（如含水方块）同样返回 PASS；
    // 生成失败（如和平难度禁止的实体）返回 SUCCESS 但不消耗物品。
    if (!spawnEntity(world, hitPos, world::spawn::SpawnReason::SpawnEgg)) {
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    world.gameEvent(gameevent::GameEvents::ENTITY_PLACE, hitPos, nullptr);
    if (!player.isCreative()) {
        player.getHeldItem(hand).shrink(1);
    }
    // 对齐 Java awardStat(Stats.ITEM_USED)
    player.awardUsedStat(this->itemLocation(), 1);
    return ItemActionResult::success(player.getHeldItem(hand));
}

bool SpawnEggItem::spawnEntity(IWorld& world, const BlockPos& pos, world::spawn::SpawnReason spawnReason) const
{
    // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        return false;
    }

    // 反查真实 EntityType：刷怪蛋内持有的 EntityType 副本工厂为空（仅作名称载体），
    // 必须通过实体注册表按名称查找真实工厂，否则 create 返回 nullptr。
    const entity::EntityType* realType = entity::EntityRegistry::instance().getType(m_entityType.name());
    if (realType == nullptr) {
        return false;
    }

    // 和平难度检查（对齐 Java SpawnEggItem.isAllowedInPeaceful）：和平难度下不允许的实体不生成。
    // 用 EntityType::isAllowedInPeaceful()（NotInPeaceful 标志位，对齐 vanilla 逐实体标注），
    // 少数 Monster 类实体（Shulker/Hoglin/Piglin 等）vanilla 未标 notInPeaceful 故和平可生成。
    if (world.difficulty() == Difficulty::Peaceful && !realType->isAllowedInPeaceful()) {
        return false;
    }

    auto entity = realType->create(&world, *registry);
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
