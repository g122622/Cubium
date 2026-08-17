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

#include "BigDripleafBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// 倾斜延迟（MC源码）
static constexpr i32 TILT_DELAY_UNSTABLE = 10; // NONE→UNSTABLE后等待10tick
static constexpr i32 TILT_DELAY_PARTIAL = 10;  // UNSTABLE→PARTIAL后等待10tick
static constexpr i32 TILT_DELAY_FULL = 100;    // PARTIAL→FULL后等待100tick

BigDripleafBlock::BigDripleafBlock(const BlockProperties& properties)
    : Block(properties)
    , m_fullShape(CollisionShape::fullBlock())
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::TILT())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::None)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void BigDripleafBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState BigDripleafBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction horizontalFacing = context.horizontalDirection();
    BlockState state = defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), horizontalFacing);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

bool BigDripleafBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }
    return belowState->is(this) || belowState->is(VanillaBlocks::BIG_DRIPLEAF_STEM) ||
        BlockTags::BIG_DRIPLEAF_PLACEABLE().contains(*belowState);
}

BlockState BigDripleafBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 下方支撑失效时销毁自身
    if (facing == Direction::Down && !isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
        return VanillaBlocks::AIR->defaultState();
    }

    // 含水时调度水流tick
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 当上方也是大滴叶时，自身转换为大滴叶茎
    if (facing == Direction::Up && facingState.is(this)) {
        const BlockState& stemState =
            VanillaBlocks::BIG_DRIPLEAF_STEM->defaultState()
                .with(BlockStateProperties::HORIZONTAL_FACING(), state.get(BlockStateProperties::HORIZONTAL_FACING()))
                .with(BlockStateProperties::WATERLOGGED(), state.get(BlockStateProperties::WATERLOGGED()));
        return stemState;
    }

    return state;
}

const CollisionShape& BigDripleafBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // TODO: getShape（渲染/选择形状）对齐 vanilla makeShapes（Shapes.or(SHAPE_LEAF, 茎连接 shape)，
    //   按 facing 旋转的茎连接 + 按 tilt 的叶片薄层）。当前返回完整方块形状（m_fullShape），仅影响
    //   渲染命中箱与选择高亮，不影响行为（行为用 getCollisionShape）。待渲染形状系统完善后补全。
    return m_fullShape;
}

const CollisionShape& BigDripleafBlock::getCollisionShape(const BlockState& state) const
{
    // 对齐 vanilla BigDripleafBlock.SHAPE_LEAF（getCollisionShape，BigDripleafBlock.java:60-71,263-265）：
    //   NONE/UNSTABLE → Block.column(16, 11, 15)：x,z 全宽（0~16 像素），y=11~15 像素的顶部薄层
    //     （11/16=0.6875 ~ 15/16=0.9375）。实体站在叶片顶面，脚 y≈pos.y+15/16，落入叶片格内触发
    //     doBlockCollisions→onEntityCollision。
    //   PARTIAL → Block.column(16, 11, 13)：y=11~13 像素（更薄，部分倾斜）。
    //   FULL → Shapes.empty()：无碰撞，实体穿透下落（wiki「倾斜后实体摔落」）。
    //
    // 【关键修复】旧实现 NONE/UNSTABLE/PARTIAL 均返回 m_fullShape（完整方块 0~1），与 vanilla 顶部薄层
    //   偏差。完整方块碰撞箱使实体脚停在叶片格上方 y=pos.y+1.0（fullBlock 顶），实体 AABB 不覆盖叶片格，
    //   doBlockCollisions 不遍历叶片 → onEntityCollision 永不触发 → 实体踩踏无法触发倾斜状态机
    //   （wiki「实体停留触发倾斜」失效）。改为顶部薄层后，实体脚停在 y=pos.y+15/16（叶片顶），AABB 覆盖
    //   叶片格，onEntityCollision 正常触发，canEntityTilt（position.y > pos.y+0.6875）满足。
    //
    // 返回 const 引用：用函数内 static 缓存按 tilt 的形状（首次调用构造，Cubium 主 tick 单线程安全）。
    // 不用成员（m_fullShape 仅一份，叶子有 3 种 tilt 碰撞箱需多份缓存）。
    static const CollisionShape s_noneShape = CollisionShape::fromPixelBox(0.0f, 11.0f, 0.0f, 16.0f, 15.0f, 16.0f);
    static const CollisionShape s_partialShape = CollisionShape::fromPixelBox(0.0f, 11.0f, 0.0f, 16.0f, 13.0f, 16.0f);

    BlockStateProperties::Tilt tilt = state.get(BlockStateProperties::TILT());
    switch (tilt) {
        case BlockStateProperties::Tilt::Full:
            // 完全倾斜时无碰撞，实体穿透下落
            return VoxelShapes::empty();
        case BlockStateProperties::Tilt::Partial:
            return s_partialShape;
        case BlockStateProperties::Tilt::None:
        case BlockStateProperties::Tilt::Unstable:
        default:
            return s_noneShape;
    }
}

const fluid::FluidState* BigDripleafBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

void BigDripleafBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 红石信号激活时立即重置倾斜状态
    if (world::redstone::RedstonePower::isPowered(world, pos)) {
        _resetTilt(world, pos, state);
        return;
    }

    BlockStateProperties::Tilt tilt = state.get(BlockStateProperties::TILT());

    switch (tilt) {
        case BlockStateProperties::Tilt::Unstable:
            // UNSTABLE → PARTIAL
            _setTiltAndScheduleTick(world, pos, state, BlockStateProperties::Tilt::Partial, true);
            break;

        case BlockStateProperties::Tilt::Partial:
            // PARTIAL → FULL
            _setTiltAndScheduleTick(world, pos, state, BlockStateProperties::Tilt::Full, true);
            break;

        case BlockStateProperties::Tilt::Full:
            // FULL → NONE (自动重置)
            _resetTilt(world, pos, state);
            break;

        default:
            break;
    }
}

void BigDripleafBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    // 大滴叶不需要随机刻
}

void BigDripleafBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 当接收到红石信号时，立即重置倾斜状态为NONE
    if (world::redstone::RedstonePower::isPowered(world, pos)) {
        BlockState currentState = *world.getBlockState(pos);
        _resetTilt(world, pos, currentState);
    }
}

void BigDripleafBlock::onProjectileHit(
    IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile)
{
    MC_UNUSED(hitResult);
    MC_UNUSED(projectile);

    // 投掷物击中时直接设为FULL倾斜
    // 注意：投掷物击中不受红石信号影响，即使有红石信号也会设为FULL
    // 但下一次tick会因为红石信号而立即重置为NONE
    BlockState mutableState = state;
    _setTiltAndScheduleTick(world, hitResult.blockPos(), mutableState, BlockStateProperties::Tilt::Full, true);
}

const BlockState& BigDripleafBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& BigDripleafBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), mirrored);
}

void BigDripleafBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 只有NONE状态的叶片才触发倾斜
    // 实体必须站在地面上且位于方块上方0.6875以上
    // 红石信号激活时不允许实体触发倾斜
    if (state.get(BlockStateProperties::TILT()) == BlockStateProperties::Tilt::None && _canEntityTilt(pos, entity) &&
        !world::redstone::RedstonePower::isPowered(world, pos)) {
        // 设置为UNSTABLE并调度tick（无音效，与MC原版一致）
        BlockState mutableState = state;
        _setTilt(world, pos, mutableState, BlockStateProperties::Tilt::Unstable);
        _scheduleTiltTick(world, pos, BlockStateProperties::Tilt::Unstable);
    }
}

// ========== 私有方法 ==========

i32 BigDripleafBlock::_getTiltDelay(BlockStateProperties::Tilt tilt)
{
    switch (tilt) {
        case BlockStateProperties::Tilt::Unstable:
            return TILT_DELAY_UNSTABLE;
        case BlockStateProperties::Tilt::Partial:
            return TILT_DELAY_PARTIAL;
        case BlockStateProperties::Tilt::Full:
            return TILT_DELAY_FULL;
        default:
            return 0;
    }
}

void BigDripleafBlock::_scheduleTiltTick(IWorld& world, const BlockPos& pos, BlockStateProperties::Tilt tilt) const
{
    i32 delay = _getTiltDelay(tilt);
    if (delay > 0) {
        world.tickManager().scheduleBlockTick(pos, const_cast<BigDripleafBlock&>(*this), delay);
    }
}

void BigDripleafBlock::_setTiltAndScheduleTick(
    IWorld& world, const BlockPos& pos, BlockState& state, BlockStateProperties::Tilt tilt, bool playSound)
{
    _setTilt(world, pos, state, tilt);

    if (playSound) {
        f32 pitch = 0.8f + world.getRandom().nextFloat() * 0.4f;
        world.playSound(
            SoundEvents::BLOCK_BIG_DRIPLEAF_TILT_DOWN, sound::SoundCategory::Blocks, pos.center(), 1.0f, pitch);
    }

    _scheduleTiltTick(world, pos, tilt);
}

void BigDripleafBlock::_setTilt(IWorld& world, const BlockPos& pos, BlockState& state, BlockStateProperties::Tilt tilt)
{
    BlockStateProperties::Tilt oldTilt = state.get(BlockStateProperties::TILT());
    BlockState newState = state.with(BlockStateProperties::TILT(), tilt);
    world.setBlockState(pos, &newState, 2);
    state = newState;

    // FULL倾斜会触发振动事件（通知幽匿感测体）
    if (tilt == BlockStateProperties::Tilt::Full && tilt != oldTilt) {
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &newState);
    }
}

void BigDripleafBlock::_resetTilt(IWorld& world, const BlockPos& pos, BlockState& state)
{
    BlockStateProperties::Tilt oldTilt = state.get(BlockStateProperties::TILT());
    _setTilt(world, pos, state, BlockStateProperties::Tilt::None);

    // 如果之前不是NONE状态，播放重置音效
    if (oldTilt != BlockStateProperties::Tilt::None) {
        f32 pitch = 0.8f + world.getRandom().nextFloat() * 0.4f;
        world.playSound(
            SoundEvents::BLOCK_BIG_DRIPLEAF_TILT_UP, sound::SoundCategory::Blocks, pos.center(), 1.0f, pitch);
    }
}

bool BigDripleafBlock::_canEntityTilt(const BlockPos& pos, const Entity& entity)
{
    return entity.onGround() && entity.position().y > static_cast<f32>(pos.y) + ENTITY_DETECTION_MIN_Y;
}

} // namespace blocks
} // namespace mc
