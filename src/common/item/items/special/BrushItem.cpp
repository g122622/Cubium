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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/item/items/special/BrushItem.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/ray/Ray.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/functional/TrailsBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/BrushableBlockEntity.hpp"
#include <utility>

namespace mc {
namespace item {

// ============================================================================
// DustParticlesDelta
// ============================================================================

BrushItem::DustParticlesDelta BrushItem::DustParticlesDelta::fromDirection(
    const Vector3& viewVector, Direction direction) noexcept
{
    // 对齐 MC 1.21.11 BrushItem.DustParticlesDelta.fromDirection
    // ALONG_SIDE_DELTA = 1.0, OUT_FROM_SIDE_DELTA = 0.1
    switch (direction) {
        case Direction::Down:
        case Direction::Up:
            // 顶/底面：粒子方向偏移取视线向量的 (z, 0, -x)
            return DustParticlesDelta{static_cast<f64>(viewVector.z), 0.0, -static_cast<f64>(viewVector.x)};
        case Direction::North:
            return DustParticlesDelta{1.0, 0.0, -0.1};
        case Direction::South:
            return DustParticlesDelta{-1.0, 0.0, 0.1};
        case Direction::West:
            return DustParticlesDelta{-0.1, 0.0, -1.0};
        case Direction::East:
            return DustParticlesDelta{0.1, 0.0, 1.0};
        default:
            return DustParticlesDelta{0.0, 0.0, 0.0};
    }
}

// ============================================================================
// 构造与基本属性
// ============================================================================

BrushItem::BrushItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ActionResultType BrushItem::onItemUse(ItemUseContext& context)
{
    Player* player = context.getPlayer();
    if (player != nullptr) {
        // 对齐 MC BrushItem.useOn：仅当视线对准方块时才开始持续使用
        const BlockRaycastResult hit = calculateHitResult(*player);
        if (hit.isHit()) {
            player->setActiveHand(context.getHand());
        }
    }
    return ActionResultType::Consume;
}

ItemActionResult BrushItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    MC_UNUSED(world);
    player.setActiveHand(hand);
    return ItemActionResult::success(
        player.getMutableEquipment(hand == Hand::MainHand ? EquipmentSlot::MainHand : EquipmentSlot::OffHand));
}

// ============================================================================
// onUseTick 核心刷扫逻辑
// ============================================================================

void BrushItem::onUseTick(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 elapsedTicks)
{
    // 仅玩家可以使用刷子
    auto* player = dynamic_cast<Player*>(&entity);
    if (player == nullptr) {
        entity.stopActiveHand();
        return;
    }

    // 计算当前是否为刷扫触发tick
    // 对齐 MC 1.21.11 BrushItem.onUseTick：
    //   int i = getUseDuration - count + 1;  // count 即剩余时间，从 useDuration 递减
    //   boolean flag = i % 10 == 5;
    // 其中 i 即为本项目的 elapsedTicks（1-based）。
    // 当 (elapsedTicks % ANIMATION_DURATION == BRUSH_TICK_IN_CYCLE + 1) 时触发刷扫
    // 即 elapsedTicks = 5, 15, 25, 35, ...
    const bool shouldBrush = (elapsedTicks % ANIMATION_DURATION == BRUSH_TICK_IN_CYCLE + 1);
    if (!shouldBrush) {
        return;
    }

    // 对齐 MC BrushItem.onUseTick：先做视线射线检测
    const BlockRaycastResult hit = calculateHitResult(*player);
    if (!hit.isHit()) {
        // 未对准方块，取消使用（对应 MC releaseUsingItem）
        entity.stopActiveHand();
        return;
    }

    const BlockPos& blockPos = hit.blockPos();
    const BlockState* blockState = world.getBlockState(blockPos);
    if (blockState == nullptr) {
        return;
    }

    // 对齐 MC：判断主手/副手决定粒子方向镜像
    // 主手使用时取主手臂侧；副手使用时取主手臂的对侧
    const HandSide primaryArm = player->getPrimaryHand();
    const HandSide arm = (player->getActiveHand() == Hand::MainHand)
        ? primaryArm
        : (primaryArm == HandSide::Right ? HandSide::Left : HandSide::Right);

    // 对齐 MC：shouldSpawnTerrainParticles() && getRenderShape() != INVISIBLE
    // 才生成方块碎屑粒子
    if (blockState->shouldSpawnTerrainParticles() && !blockState->isInvisibleRenderType()) {
        spawnDustParticles(world, hit, *blockState, player->getLookVector(), arm);
    }

    // 对齐 MC：播放刷扫音效
    // 若命中方块是 BrushableBlock，使用其绑定的刷扫音效；否则使用 BRUSH_GENERIC
    const auto* brushableBlock = dynamic_cast<const blocks::BrushableBlock*>(&blockState->getBlock());
    const ResourceLocation& brushSound =
        (brushableBlock != nullptr) ? brushableBlock->getBrushSound() : SoundEvents::BRUSH_GENERIC;

    // world.playSound(player, blockpos, soundevent, SoundSource.BLOCKS)
    world.playSound(brushSound, sound::SoundCategory::Blocks, blockPos.center(), 1.0f, 1.0f);

    // 对齐 MC 1.21.11 BrushItem.onUseTick：
    // 仅在服务端且命中方块实体为 BrushableBlockEntity 时调用 brush()
    if (brushableBlock != nullptr && !world.isClientSide()) {
        BlockEntity* blockEntity = world.getBlockEntity(blockPos);
        if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::BrushableBlock) {
            auto* brushableEntity = static_cast<blockentity::BrushableBlockEntity*>(blockEntity);
            const Direction hitDirection = hit.face();
            const i64 gameTime = static_cast<i64>(world.getGameTime());

            // 调用 brush()，返回 true 表示刷扫完成（刷出物品并转换方块）
            const bool completed = brushableEntity->brush(gameTime, world, entity, hitDirection, stack);

            if (completed) {
                // 刷扫完成时消耗1耐久
                // 对齐 MC: stack.hurtAndBreak(1, player, equipmentslot)
                // equipmentslot 由当前活动手决定
                const Hand activeHand = entity.getActiveHand();
                LivingEntity::hurtAndBreak(stack, 1, &entity, LivingEntity::handToEquipmentSlot(activeHand));
            }
        }
    }
}

// ============================================================================
// 私有辅助方法
// ============================================================================

BlockRaycastResult BrushItem::calculateHitResult(const Player& player)
{
    // 对齐 MC 1.21.11 BrushItem.calculateHitResult：
    //   ProjectileUtil.getHitResultOnViewVector(player, EntitySelector.CAN_BE_PICKED, player.blockInteractionRange())
    // 本项目仅做方块射线检测（刷子主要场景为方块），距离取自 Player::blockInteractionRange()，
    // 该值由 generic.block_interaction_range 属性决定（生存/冒险 4.5，创造 5.0）。
    const Vector3 eyePosition = player.getEyePosition();
    const Ray ray(eyePosition, player.getLookVector());
    const RaycastContext context(ray, static_cast<f32>(player.blockInteractionRange()));
    return raycastBlocks(context, *player.world());
}

void BrushItem::spawnDustParticles(IWorld& world,
    const BlockRaycastResult& hitResult,
    const BlockState& blockState,
    const Vector3& viewVector,
    HandSide arm)
{
    // 对齐 MC 1.21.11 BrushItem.spawnDustParticles
    constexpr f64 ALONG_SIDE_DELTA = 3.0; // MC 源码中 d0 = 3.0
    const i32 directionSign = (arm == HandSide::Right) ? 1 : -1;

    // MC: int j = random.nextInt(7, 12);  // [7, 12)
    // Cubium nextInt(min, max) 是 [min, max] 闭区间，因此用 nextInt(7, 11) 等价 [7, 11] = [7, 12)
    math::Random& rng = world.getRandom();
    const i32 particleCount = rng.nextInt(7, 11);

    const Direction direction = hitResult.face();
    const DustParticlesDelta delta = DustParticlesDelta::fromDirection(viewVector, direction);
    const Vector3 hitPos = hitResult.hitPosition();

    // 对齐 MC：West 方向时 x 坐标减 1e-6，North 方向时 z 坐标减 1e-6
    const f32 xOffset = (direction == Direction::West) ? -1.0e-6f : 0.0f;
    const f32 zOffset = (direction == Direction::North) ? -1.0e-6f : 0.0f;

    for (i32 k = 0; k < particleCount; ++k) {
        // MC: addParticle(blockparticleoption, vec3.x - offset, vec3.y, vec3.z - offset,
        //                 delta.xd * sign * 3.0 * random.nextDouble(),
        //                 0.0,
        //                 delta.zd * sign * 3.0 * random.nextDouble());
        const Vector3 position(hitPos.x + xOffset, hitPos.y, hitPos.z + zOffset);
        const f64 velocityX = delta.xd * directionSign * ALONG_SIDE_DELTA * rng.nextDouble();
        const f64 velocityZ = delta.zd * directionSign * ALONG_SIDE_DELTA * rng.nextDouble();
        const Vector3 velocity(static_cast<f32>(velocityX), 0.0f, static_cast<f32>(velocityZ));

        // 使用 Block 粒子携带方块状态（等价 MC 的 BlockParticleOption(ParticleTypes.BLOCK, blockstate)）
        world.addBlockParticle(particle::ParticleTypeId::Block, position, velocity, blockState);
    }
}

// ============================================================================
// 实体交互（犰狳）与其他
// ============================================================================

bool BrushItem::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand)
{
    // TODO: 当 ArmadilloEntity 实现后，在此处添加刷犰狳的逻辑：
    // 1. 检查 target 是否为 ArmadilloEntity
    // 2. 检查犰狳是否处于可刷状态（scuteCooldown == 0 且非幼年）
    // 3. 掉落 armadillo_scute 物品
    // 4. 播放刷犰狳音效 (ARMADILLO_BRUSH)
    // 5. 消耗 ARMADILLO_DURABILITY_COST (16) 耐久:
    //    LivingEntity::hurtAndBreak(stack, ARMADILLO_DURABILITY_COST, &player,
    //        LivingEntity::handToEquipmentSlot(hand));
    // 6. 设置犰狳的 scuteCooldown
    //
    // MC原版逻辑参考: Armadillo.mobInteract() 和 DispenseItemBehavior 中的刷子逻辑

    MC_UNUSED(stack);
    MC_UNUSED(player);
    MC_UNUSED(target);
    MC_UNUSED(hand);
    return false;
}

i32 BrushItem::getUseDuration(const ItemStack& /*stack*/) const
{
    return USE_DURATION;
}

} // namespace item
} // namespace mc
