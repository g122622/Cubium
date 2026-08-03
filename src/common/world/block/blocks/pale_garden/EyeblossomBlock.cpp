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

#include "EyeblossomBlock.hpp"

#include <cmath>

#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/special/BeeEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/TriState.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/block/blocks/pale_garden/EyeblossomEnvironment.hpp"
#include "common/world/block/blocks/vegetation/FlowerBlock.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// 构造函数
// ============================================================================

EyeblossomBlock::EyeblossomBlock(
    const BlockProperties& properties, Type type, u32 suspiciousStewEffect, i32 effectDuration)
    : FlowerBlock(properties, suspiciousStewEffect, effectDuration)
    , m_type(type)
{}

// ============================================================================
// 光照
// ============================================================================

u8 EyeblossomBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 开放状态发光等级为 1，闭合状态为 0
    return m_type == Type::Open ? 1 : 0;
}

// ============================================================================
// 随机刻 / 计划刻
// ============================================================================

void EyeblossomBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 客户端世界不处理状态切换
    if (world.isClientSide()) {
        return;
    }

    if (tryChangingState(world, pos, state, random)) {
        // 随机刻触发使用长音效
        const Type newType = m_type == Type::Open ? Type::Closed : Type::Open;
        world.playSound(longSwitchSoundOf(newType), sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
    }
}

void EyeblossomBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 客户端世界不处理状态切换
    if (world.isClientSide()) {
        return;
    }

    if (tryChangingState(world, pos, state, random)) {
        // 连锁 tick 触发使用短音效
        const Type newType = m_type == Type::Open ? Type::Closed : Type::Open;
        world.playSound(shortSwitchSoundOf(newType), sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
    }
}

// ============================================================================
// 客户端动画
// ============================================================================

void EyeblossomBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    MC_UNUSED(state);

    // 仅开放状态在苍白苔藓方块上偶尔播放环境音
    if (m_type != Type::Open) {
        return;
    }

    // 1/700 概率触发
    if (random.nextInt(700) != 0) {
        return;
    }

    // 检查下方是否为苍白苔藓方块（minecraft:pale_moss_block）
    const BlockState* belowState = context.getBlockState(pos.x, pos.y - 1, pos.z);
    if (belowState == nullptr) {
        return;
    }

    const Block& belowBlock = belowState->getBlock();
    if (belowBlock.blockLocation() != ResourceLocation("minecraft", "pale_moss_block")) {
        return;
    }

    context.playLocalSound(SoundEvents::BLOCK_EYEBLOSSOM_IDLE, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
}

// ============================================================================
// 实体碰撞
// ============================================================================

void EyeblossomBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(pos);

    // 1. 客户端世界不处理状态变更
    if (world.isClientSide()) {
        return;
    }

    // 2. 和平难度不施加中毒效果
    if (world.difficulty() == Difficulty::Peaceful) {
        return;
    }

    // 3. 仅蜜蜂被眼眸花中毒影响（其他实体不触发）
    auto* bee = dynamic_cast<BeeEntity*>(&entity);
    if (bee == nullptr) {
        return;
    }

    // 4. 仅吸引蜜蜂的花朵触发（开放眼眸花在 BEE_ATTRACTIVE 标签中，闭合眼眸花不在）
    //    含水花与向日葵下半部分由 attractsBees 排除
    if (!BeeEntity::attractsBees(state)) {
        return;
    }

    // 5. 蜜蜂已中毒则跳过，避免重复施加刷新剩余时间
    if (bee->hasEffect(entity::effect::EffectType::Poison)) {
        return;
    }

    // 6. 施加 25 tick 中毒效果（Poison I，amplifier=0）
    bee->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 25, 0));
}

// ============================================================================
// 类型查询
// ============================================================================

const EyeblossomBlock* EyeblossomBlock::transform() const noexcept
{
    // 通过 BlockRegistry 反查反状态方块
    const ResourceLocation& currentId = blockLocation();
    ResourceLocation oppositeId;
    if (currentId == ResourceLocation("minecraft", "open_eyeblossom")) {
        oppositeId = ResourceLocation("minecraft", "closed_eyeblossom");
    } else if (currentId == ResourceLocation("minecraft", "closed_eyeblossom")) {
        oppositeId = ResourceLocation("minecraft", "open_eyeblossom");
    } else {
        return nullptr;
    }

    Block* oppositeBlock = BlockRegistry::instance().getBlock(oppositeId);
    if (oppositeBlock == nullptr) {
        return nullptr;
    }
    return dynamic_cast<const EyeblossomBlock*>(oppositeBlock);
}

const ResourceLocation& EyeblossomBlock::longSwitchSoundOf(Type type) noexcept
{
    switch (type) {
        case Type::Open:
            return SoundEvents::BLOCK_EYEBLOSSOM_OPEN_LONG;
        case Type::Closed:
            return SoundEvents::BLOCK_EYEBLOSSOM_CLOSE_LONG;
    }
    // 不可达，返回一个有效的引用以避免 UB
    return SoundEvents::BLOCK_EYEBLOSSOM_OPEN_LONG;
}

const ResourceLocation& EyeblossomBlock::shortSwitchSoundOf(Type type) noexcept
{
    switch (type) {
        case Type::Open:
            return SoundEvents::BLOCK_EYEBLOSSOM_OPEN;
        case Type::Closed:
            return SoundEvents::BLOCK_EYEBLOSSOM_CLOSE;
    }
    // 不可达，返回一个有效的引用以避免 UB
    return SoundEvents::BLOCK_EYEBLOSSOM_OPEN;
}

// ============================================================================
// 转换粒子
// ============================================================================

void EyeblossomBlock::spawnTransformParticle(IWorld& world, const BlockPos& pos, math::IRandom& random, Type newType)
{
    // 客户端世界不生成粒子（粒子由客户端 animateTick 自行处理，或由服务端广播）
    if (world.isClientSide()) {
        return;
    }

    // 中心点 + 随机偏移目标点，匹配 MC 1.21.11 EyeblossomBlock.Type#spawnTransformParticle
    // double d0 = 0.5 + random.nextDouble();
    // Vec3 offset = (random.nextDouble() - 0.5, random.nextDouble() + 1.0, random.nextDouble() - 0.5);
    // Vec3 target = center.add(offset.scale(d0));
    // TrailParticleOption option = new TrailParticleOption(target, particleColor, (int)(20.0 * d0));
    const Vector3 center = pos.center();
    const f64 d0 = 0.5 + random.nextDouble();
    const f64 offsetX = random.nextDouble() - 0.5;
    const f64 offsetY = random.nextDouble() + 1.0;
    const f64 offsetZ = random.nextDouble() - 0.5;
    const Vector3d offset(offsetX, offsetY, offsetZ);
    const Vector3d target = Vector3d(center.x, center.y, center.z) + offset * d0;
    const i32 durationInTicks = static_cast<i32>(20.0 * d0);
    const u32 color = particleColorOf(newType);

    // 通过 IWorld 虚方法广播给附近玩家（ServerWorld 重写以实际广播）
    world.addTrailParticle(center, target, color, durationInTicks);
}

// ============================================================================
// 内部状态切换
// ============================================================================

bool EyeblossomBlock::tryChangingState(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 1. 读取 EYEBLOSSOM_OPEN 环境属性
    //    TriState::Default 时回退到当前方块状态（即不切换）
    const util::TriState envOpen = eyeblossom_environment::getEyeblossomOpen(world, pos);
    const bool currentOpen = (m_type == Type::Open);
    const bool targetOpen = util::triStateToBoolean(envOpen, currentOpen);

    // 2. 若环境偏好与当前状态一致，则不切换
    if (targetOpen == currentOpen) {
        return false;
    }

    // 3. 切换为反状态方块
    const EyeblossomBlock* newBlock = transform();
    if (newBlock == nullptr) {
        return false;
    }
    const BlockState& newState = newBlock->defaultState();
    world.setBlockState(pos, &newState, 3);

    // 4. 触发 BLOCK_CHANGE 游戏事件（供幽匿感测体感知）
    world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &state);

    // 5. 生成转换粒子（粒子颜色由新状态决定）
    spawnTransformParticle(world, pos, random, newBlock->m_type);

    // 6. 连锁触发周围 3×2×3 范围内同种眼眸花方块
    //    延迟 = random.nextIntBetweenInclusive((int)(dist*5), (int)(dist*10))
    //    匹配 MC 1.21.11: BlockPos.betweenClosed(pos.offset(-3,-2,-3), pos.offset(3,2,3))
    const BlockPos startPos(pos.x - 3, pos.y - 2, pos.z - 3);
    const BlockPos endPos(pos.x + 3, pos.y + 2, pos.z + 3);
    startPos.forEachBetween(endPos, [&](const BlockPos& neighborPos) -> bool {
        // 跳过自身
        if (neighborPos == pos) {
            return true;
        }

        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState == nullptr) {
            return true;
        }

        // 仅对"与切换前状态相同"的眼眸花方块调度 tick（指针比较）
        // 这样邻居会在延迟 tick 后调用 tick()，进而再次走 tryChangingState 流程
        if (neighborState != &state) {
            return true;
        }

        const f64 dist = std::sqrt(static_cast<f64>(pos.distanceSq(neighborPos)));
        const i32 minDelay = static_cast<i32>(dist * 5.0);
        const i32 maxDelay = static_cast<i32>(dist * 10.0);
        const i32 delay = random.nextInt(minDelay, maxDelay);

        // 仅在尚未调度时添加，避免重复 tick
        auto& tickManager = world.tickManager();
        if (!tickManager.isBlockTickScheduled(neighborPos, *this)) {
            tickManager.scheduleBlockTick(neighborPos, *this, delay, world::tick::TickPriority::Normal);
        }
        return true;
    });

    return true;
}

} // namespace blocks
} // namespace mc
