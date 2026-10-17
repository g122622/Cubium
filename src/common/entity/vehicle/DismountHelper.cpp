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

#include "DismountHelper.hpp"

#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/decorative/CampfireBlock.hpp"
#include "common/world/block/blocks/building/TrapDoorBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include <cmath>
#include <limits>

namespace mc {

namespace {

/**
 * @brief 获取方块柱的地板碰撞形状
 *
 * 可攀爬方块（梯子、藤蔓、脚手架等）与开启的活板门不提供地板：实体可以从它们"穿过去"落地。
 *
 * @param world 世界
 * @param pos 方块柱位置
 * @return 地板碰撞形状；无可通行形状时返回空形状
 */
[[nodiscard]] const CollisionShape& _nonClimbableShape(const IWorld& world, const BlockPos& pos)
{
    static const CollisionShape EMPTY;

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return EMPTY;
    }
    // 标签系统尚未初始化（单元测试夹具）时 getTag 返回 nullptr，按"不在标签中"处理。
    if (BlockTags::isInitialized() && BlockTags::CLIMBABLE().contains(*state)) {
        return EMPTY;
    }
    const auto* trapdoor = dynamic_cast<const blocks::TrapDoorBlock*>(&state->owner());
    if (trapdoor != nullptr && state->hasProperty(BlockStateProperties::OPEN()) &&
        state->get(BlockStateProperties::OPEN())) {
        return EMPTY;
    }
    const CollisionShape& shape = state->owner().getCollisionShape(*state);
    // 无碰撞箱的方块（空气、液体、火、植物等）不构成地板，否则会被当成"顶面高度 0 的实心地板"。
    return shape.isEmpty() ? EMPTY : shape;
}

/**
 * @brief 计算碰撞形状在 Y 轴上的最高点
 *
 * 对应 vanilla VoxelShape.max(Axis.Y)；空形状返回 0。
 */
[[nodiscard]] f64 _shapeMaxY(const CollisionShape& shape) noexcept
{
    f64 maxY = 0.0;
    for (const auto& box : shape.boxes()) {
        maxY = std::max(maxY, static_cast<f64>(box.maxY));
    }
    return maxY;
}

} // namespace

bool DismountHelper::isBlockFloorValid(f64 floorHeight) noexcept
{
    return std::isfinite(floorHeight) && floorHeight < 1.0;
}

f64 DismountHelper::getBlockFloorHeight(const IWorld& world, const BlockPos& pos, f32 width, f32 height)
{
    MC_UNUSED(width);
    MC_UNUSED(height);

    const CollisionShape& ownShape = _nonClimbableShape(world, pos);
    if (!ownShape.isEmpty()) {
        return _shapeMaxY(ownShape);
    }

    // 方块自身不提供地板：向下一格取地板高度，若下方方块也完全可穿过则视为无地面。
    const CollisionShape& belowShape = _nonClimbableShape(world, pos.down());
    const f64 belowMaxY = _shapeMaxY(belowShape);
    if (belowMaxY >= 1.0) {
        return belowMaxY - 1.0;
    }
    return std::numeric_limits<f64>::infinity();
}

bool DismountHelper::isBlockDangerous(const BlockState& state, bool fireImmune)
{
    if (!fireImmune) {
        const bool burning = (BlockTags::isInitialized() && BlockTags::FIRE().contains(state)) ||
            state.is(VanillaBlocks::LAVA) ||
            state.is(VanillaBlocks::MAGMA) || blocks::CampfireBlock::isLitCampfire(state) ||
            state.is(VanillaBlocks::LAVA_CAULDRON);
        if (burning) {
            return true;
        }
    }
    return state.is(VanillaBlocks::WITHER_ROSE) || state.is(VanillaBlocks::SWEET_BERRY_BUSH) ||
        state.is(VanillaBlocks::CACTUS) || state.is(VanillaBlocks::POWDER_SNOW);
}

std::optional<Vector3> DismountHelper::findSafeDismountLocation(const IWorld& world,
    const BlockPos& pos,
    f32 width,
    f32 height,
    bool isPlayer,
    bool fireImmune,
    bool safeMode)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return std::nullopt;
    }

    if (safeMode && isBlockDangerous(*state, fireImmune)) {
        return std::nullopt;
    }

    const f64 floorHeight = getBlockFloorHeight(world, pos, width, height);
    if (!isBlockFloorValid(floorHeight)) {
        return std::nullopt;
    }

    if (safeMode && floorHeight <= 0.0) {
        const BlockState* below = world.getBlockState(pos.down());
        if (below != nullptr && isBlockDangerous(*below, fireImmune)) {
            return std::nullopt;
        }
    }

    const Vector3 bottomCenter(static_cast<f32>(pos.x) + 0.5f,
        static_cast<f32>(pos.y) + static_cast<f32>(floorHeight),
        static_cast<f32>(pos.z) + 0.5f);
    const AxisAlignedBB box = AxisAlignedBB::fromPosition(bottomCenter, width, height);

    if (world.hasBlockCollision(box)) {
        return std::nullopt;
    }

    // 玩家的额外限制：末地传送门/末地折跃门内部不允许重生。
    if (isPlayer) {
        const BlockState* above = world.getBlockState(pos.up());
        if (BlockTags::isInitialized() &&
            (BlockTags::INVALID_SPAWN_INSIDE().contains(*state) ||
                (above != nullptr && BlockTags::INVALID_SPAWN_INSIDE().contains(*above)))) {
            return std::nullopt;
        }
    }

    if (!world.worldBorder().isWithinBounds(box)) {
        return std::nullopt;
    }

    return bottomCenter;
}

} // namespace mc