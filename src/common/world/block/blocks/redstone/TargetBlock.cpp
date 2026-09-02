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

#include "TargetBlock.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

TargetBlock::TargetBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWER_0_15())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::POWER_0_15(), 0));
}

i32 TargetBlock::getPower(const BlockState& state)
{
    return state.get(BlockStateProperties::POWER_0_15());
}

BlockState TargetBlock::withPower(BlockState state, i32 power)
{
    power = std::max(0, std::min(power, 15));
    return state.with(BlockStateProperties::POWER_0_15(), power);
}

i32 TargetBlock::getRedstoneStrength(const Vector3& hitPos, Direction hitFace)
{
    // 对齐 vanilla TargetBlock.getRedstoneStrength（TargetBlock.java:61-77）。
    // 取命中点在命中面平面内两个坐标轴的小数偏移最大值 d3：
    //   - 命中顶/底面（Y 轴）：取 X、Z 偏移最大值
    //   - 命中前/后面（Z 轴）：取 X、Y 偏移最大值
    //   - 命中左/右面（X 轴）：取 Y、Z 偏移最大值
    // 返回 ceil(15 * clamp((0.5 - d3) / 0.5, 0, 1))，至少为 1。
    // 越靠近面中心信号越强（正中 d3=0 → 15，边缘 d3=0.5 → 1）。
    f32 fracX = hitPos.x - std::floor(hitPos.x);
    f32 fracY = hitPos.y - std::floor(hitPos.y);
    f32 fracZ = hitPos.z - std::floor(hitPos.z);

    f32 d0 = std::abs(fracX - 0.5f); // X 偏移到中心
    f32 d1 = std::abs(fracY - 0.5f); // Y 偏移到中心
    f32 d2 = std::abs(fracZ - 0.5f); // Z 偏移到中心

    Axis axis = Directions::getAxis(hitFace);
    f32 d3;
    if (axis == Axis::Y) {
        d3 = std::max(d0, d2);
    } else if (axis == Axis::Z) {
        d3 = std::max(d0, d1);
    } else {
        d3 = std::max(d1, d2);
    }

    f32 clamped = std::clamp((0.5f - d3) / 0.5f, 0.0f, 1.0f);
    return std::max(1, static_cast<i32>(std::ceil(15.0f * clamped)));
}

void TargetBlock::onProjectileHit(
    IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile)
{
    // 对齐 vanilla TargetBlock.onProjectileHit（TargetBlock.java:42-49）+
    // updateRedstoneOutput（TargetBlock.java:51-59）。
    // 计算命中精度对应的红石信号强度，设置方块 state 并调度信号结束 tick。
    // 箭矢持续 ACTIVATION_TICKS_ARROWS(20) tick，其他投射物持续 ACTIVATION_TICKS_OTHER(8) tick。
    // 若该方块已有调度中的 tick，则不重复调度（保留首次命中的持续时间和强度）。
    const BlockPos& pos = hitResult.blockPos();
    i32 strength = getRedstoneStrength(hitResult.hitPosition(), hitResult.face());

    // 箭矢判定：对齐 vanilla (projectile instanceof AbstractArrow) ? 20 : 8
    bool isArrow = dynamic_cast<entity::AbstractArrowEntity*>(&projectile) != nullptr;
    i32 duration = isArrow ? ACTIVATION_TICKS_ARROWS : ACTIVATION_TICKS_OTHER;

    if (!world.tickManager().isBlockTickScheduled(pos, *this)) {
        BlockState newState = withPower(state, strength);
        world.setBlockState(pos, &newState, 3);
        world.tickManager().scheduleBlockTick(pos, *this, duration, world::tick::TickPriority::High);
    }

    // 通知相邻方块红石信号变化
    world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, *this);
}

void TargetBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 标靶不响应红石信号，只响应投射物命中
    MC_UNUSED(world);
    MC_UNUSED(pos);
}

void TargetBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 信号持续时间结束，重置为0（对齐 vanilla TargetBlock.tick：power != 0 时设为 0）
    i32 currentPower = getPower(state);
    if (currentPower > 0) {
        BlockState newState = withPower(state, 0);
        world.setBlockState(pos, &newState, 3);

        // 通知相邻方块红石信号变化
        world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, *this);
    }
}

i32 TargetBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return getPower(state);
}

i32 TargetBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return getPower(state);
}

} // namespace blocks
} // namespace mc
