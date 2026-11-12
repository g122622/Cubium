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

#include "common/world/block/blocks/functional/SnifferEggBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/passive/special/SnifferEntity.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "common/world/effect/base/LevelEventId.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// SnifferEggBlock
// ============================================================================

SnifferEggBlock::SnifferEggBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HATCH_0_2())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::HATCH_0_2(), 0));

    m_noCrackShape = CollisionShape::fromPixelBox(1, 0, 2, 15, 16, 14);
    m_crackedShape = CollisionShape::fromPixelBox(1, 0, 2, 15, 16, 14);
    m_hatchingShape = CollisionShape::fromPixelBox(1, 0, 2, 15, 16, 14);
}

void SnifferEggBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState SnifferEggBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
}

bool SnifferEggBlock::isPathfindable(const BlockState& state, PathComputationType type) const
{
    // 原版显式返回 false：蛋的碰撞形状随时长变化且始终不满格。
    MC_UNUSED(state);
    MC_UNUSED(type);
    return false;
}

const CollisionShape& SnifferEggBlock::getShape(const BlockState& state) const
{
    i32 hatch = state.get(BlockStateProperties::HATCH_0_2());
    if (hatch == 2) {
        return m_hatchingShape;
    }
    if (hatch == 1) {
        return m_crackedShape;
    }
    return m_noCrackShape;
}

void SnifferEggBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    // 对齐 MC SnifferEggBlock.onPlace：放置后调度首个孵化 tick。
    // MC 原版通过 ServerLevel.scheduleTick 调用，天然只在服务端执行。
    // 本项目 onBlockAdded 由 ServerWorld::setBlockState 在方块类型变化时触发，
    // 客户端路径同样会调用此回调，因此显式跳过客户端。
    if (world.isClientSide()) {
        return;
    }

    const bool flag = hatchBoost(world, pos);
    if (flag) {
        // 加速时播放蛋壳破裂粒子事件（MC levelEvent 3009）
        world.playEvent(::mc::world::LevelEventId::PARTICLES_EGG_CRACK, pos, 0);
    }

    // 孵化总时长：加速 12000 tick，常规 24000 tick；分三阶段，每段 i/3 + [0, 300) tick
    constexpr i32 REGULAR_HATCH_TIME_TICKS = 24000;
    constexpr i32 BOOSTED_HATCH_TIME_TICKS = 12000;
    constexpr i32 RANDOM_HATCH_OFFSET_TICKS = 300;
    const i32 i = flag ? BOOSTED_HATCH_TIME_TICKS : REGULAR_HATCH_TIME_TICKS;
    const i32 j = i / 3;

    // 发出 BLOCK_PLACE 游戏事件（通知附近幽匿感测体）
    world.gameEvent(gameevent::GameEvents::BLOCK_PLACE, pos, &state);

    // 调度首个孵化 tick
    const i32 delay = j + world.getRandom().nextInt(RANDOM_HATCH_OFFSET_TICKS);
    world.tickManager().scheduleBlockTick(pos, *this, delay, world::tick::TickPriority::Normal);
}

void SnifferEggBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 对齐 MC SnifferEggBlock.tick：MC 原版通过 ServerLevel.scheduleTick 调用，
    // 天然只在服务端执行。本项目 tick 由 TickManager 调度，仅在服务端 TickManager::tick 中触发，
    // 但为防御性编程与测试友好，仍在此显式检查 isClientSide。
    if (world.isClientSide()) {
        return;
    }

    const i32 hatch = state.get(BlockStateProperties::HATCH_0_2());
    if (hatch < 2) {
        // 非孵化完成分支：播放 SNIFFER_EGG_CRACK 音效并增加孵化进度
        world.playSound(SoundEvents::SNIFFER_EGG_CRACK,
            sound::SoundCategory::Blocks,
            pos.center(),
            0.7f,
            0.9f + random.nextFloat() * 0.2f);
        BlockState newState = state.with(BlockStateProperties::HATCH_0_2(), hatch + 1);
        world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_CLIENTS);

        // 调度下一阶段 tick：MC 原版通过 onPlace 在 setBlock 时重新调度，
        // 本项目 onBlockAdded 仅在方块类型变化时触发，因此需在 tick 中显式调度下一阶段。
        // 每次重新查询 hatchBoost，因为下方方块可能在阶段间被替换。
        const bool flag = hatchBoost(world, pos);
        constexpr i32 REGULAR_HATCH_TIME_TICKS = 24000;
        constexpr i32 BOOSTED_HATCH_TIME_TICKS = 12000;
        constexpr i32 RANDOM_HATCH_OFFSET_TICKS = 300;
        const i32 i = flag ? BOOSTED_HATCH_TIME_TICKS : REGULAR_HATCH_TIME_TICKS;
        const i32 j = i / 3;
        const i32 delay = j + world.getRandom().nextInt(RANDOM_HATCH_OFFSET_TICKS);
        world.tickManager().scheduleBlockTick(pos, *this, delay, world::tick::TickPriority::Normal);
    } else {
        // 孵化完成 - 生成嗅探兽幼体
        //   1. 播放 SNIFFER_EGG_HATCH 音效
        //   2. 销毁蛋方块
        //   3. 创建 Sniffer 实体，setBaby(true)（年龄 -48000，40 分钟）
        //   4. snapTo(pos.center(), wrapDegrees(random * 360), 0)
        //   5. addFreshEntity
        world.playSound(SoundEvents::SNIFFER_EGG_HATCH,
            sound::SoundCategory::Blocks,
            pos.center(),
            0.7f,
            0.9f + random.nextFloat() * 0.2f);

        // 销毁蛋方块（对齐 MC level.destroyBlock(pos, false)）
        const BlockState* airState = BlockRegistry::instance().airState();
        if (airState != nullptr) {
            world.setBlockState(pos, airState, world::BlockUpdateFlags::UPDATE_CLIENTS);
        }

        // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
        auto* registry = world.entityRegistry();
        if (registry == nullptr) {
            return;
        }

        // 创建嗅探兽幼体
        auto sniffer = std::make_unique<SnifferEntity>(EntityInstanceId(0), *registry);
        if (sniffer) {
            sniffer->setTypeId(entity::EntityTypeKeys::SNIFFER); // 工厂绕过补救：直接构造缺 typeId
            // 设置为幼体（-48000 tick，40 分钟）
            // SnifferEntity::setChild 覆盖了 AgeableEntity::setChild，设置正确的嗅探兽幼年期
            sniffer->setChild(true);

            // 对齐 MC sniffer.snapTo(vec3.x, vec3.y, vec3.z, wrapDegrees(random * 360), 0)
            // pos.center() = (x+0.5, y+0.5, z+0.5)
            Vector3 center = pos.center();
            f32 yaw = math::wrapDegrees(random.nextFloat() * 360.0f);
            sniffer->setPosition(center.x, center.y, center.z);
            sniffer->setRotation(yaw, 0.0f);

            // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化
            // MC 原版使用 EntitySpawnReason.BREEDING（蛋由繁殖产生），这里使用 Natural
            // 因为 scheduleTick 孵化更接近自然生成场景；不影响功能
            entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(world, pos);
            sniffer->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Natural);

            // 生成到世界
            world.spawnEntity(std::move(sniffer));
        }
    }
}

bool SnifferEggBlock::hatchBoost(IWorld& world, const BlockPos& pos)
{
    // 对齐 MC SnifferEggBlock.hatchBoost：检查下方方块是否在 SNIFFER_EGG_HATCH_BOOST 标签中
    const BlockState* belowState = world.getBlockState(pos.down());
    return belowState != nullptr && BlockTags::SNIFFER_EGG_HATCH_BOOST().contains(*belowState);
}

} // namespace blocks
} // namespace mc
