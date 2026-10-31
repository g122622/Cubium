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

/**
 * @file WalkNodeEvaluatorTests.cpp
 * @brief 地面寻路求值器的单元测试
 *
 * 覆盖 `WalkNodeEvaluator` 的三层判定：
 *   1. 单格原始类型 `getPathTypeFromState`（方块 / 流体 → 路径类型）；
 *   2. 静态类型 `getPathTypeStatic`（叠加「脚下是什么」与 3×3×3 危险邻域）；
 *   3. 含实体尺寸的类型 `getPathTypeOfMob`（门、铁轨、栅栏、窄实体规则）。
 *
 * 求值器只通过 `Region` 读世界，因此这里用一个方块表驱动的内存区域即可精确构造
 * 千变万化的地形，不需要启动世界。
 */

#include "common/entity/ai/pathfinding/PathType.hpp"
#include "common/entity/ai/pathfinding/Region.hpp"
#include "common/entity/ai/pathfinding/WalkNodeEvaluator.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include <unordered_map>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::pathfinding;

namespace {

/**
 * @brief 方块表驱动的内存区域
 *
 * 未显式设置的格子一律是空气、无流体、无碰撞。碰撞形状默认取「整格实心」，
 * 需要台阶 / 半砖时用 `setShapeHeight` 指定顶面高度。
 */
class TableRegion final : public Region {
public:
    static u64 key(i32 x, i32 y, i32 z)
    {
        return (static_cast<u64>(static_cast<u32>(x)) << 42) ^ (static_cast<u64>(static_cast<u32>(y)) << 21) ^
            static_cast<u64>(static_cast<u32>(z));
    }

    void setBlock(i32 x, i32 y, i32 z, const BlockState& state)
    {
        m_blocks[key(x, y, z)] = &state;
        m_shapes.erase(key(x, y, z));
    }

    /// 指定格子的碰撞形状顶面高度（0~1）；不设置时按整格实心处理。
    void setShapeHeight(i32 x, i32 y, i32 z, f32 top)
    {
        m_shapes[key(x, y, z)] = top;
    }

    void setWater(i32 x, i32 y, i32 z)
    {
        m_fluids[key(x, y, z)] = &fluid::FluidTags::WATER();
    }

    void setLava(i32 x, i32 y, i32 z)
    {
        m_fluids[key(x, y, z)] = &fluid::FluidTags::LAVA();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto found = m_blocks.find(key(x, y, z));
        if (found != m_blocks.end()) {
            return found->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const auto found = m_fluids.find(key(x, y, z));
        if (found == m_fluids.end()) {
            return VanillaBlocks::AIR->defaultState().getFluidState();
        }
        const BlockState* state = getBlockState(x, y, z);
        return state->getFluidState();
    }

    [[nodiscard]] bool hasCollision(const void* /*entity*/, const AxisAlignedBB& box) const override
    {
        // 简化：整格实心方块与盒子相交即算碰撞，部分形状按顶面高度判定。
        const i32 minX = static_cast<i32>(std::floor(box.minX));
        const i32 maxX = static_cast<i32>(std::ceil(box.maxX) - 1);
        const i32 minY = static_cast<i32>(std::floor(box.minY));
        const i32 maxY = static_cast<i32>(std::ceil(box.maxY) - 1);
        const i32 minZ = static_cast<i32>(std::floor(box.minZ));
        const i32 maxZ = static_cast<i32>(std::ceil(box.maxZ) - 1);
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 y = minY; y <= maxY; ++y) {
                for (i32 z = minZ; z <= maxZ; ++z) {
                    const BlockState* state = getBlockState(x, y, z);
                    if (state->isAir() || state->canBeReplaced()) {
                        continue;
                    }
                    const auto shape = m_shapes.find(key(x, y, z));
                    const f32 top = shape != m_shapes.end() ? shape->second : 1.0f;
                    if (top <= 0.0f) {
                        continue;
                    }
                    if (box.minY < static_cast<f32>(y) + top && box.maxY > static_cast<f32>(y)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    [[nodiscard]] i32 minBuildHeight() const override
    {
        return -64;
    }

    [[nodiscard]] i32 maxBuildHeight() const override
    {
        return 320;
    }

private:
    std::unordered_map<u64, const BlockState*> m_blocks;
    std::unordered_map<u64, const fluid::FluidTag*> m_fluids;
    std::unordered_map<u64, f32> m_shapes;
};

/**
 * @brief 受控的寻路实体
 *
 * 尺寸、跳跃、能否进水、每种路径类型的代价都可以直接指定，从而把「求值器算得对不对」
 * 与「某个具体生物配置得对不对」分开验证。
 */
class FakeEntity final : public PathfindingEntity {
public:
    FakeEntity(f32 width, f32 height, f32 stepHeight = 0.6f)
        : m_width(width)
        , m_height(height)
        , m_stepHeight(stepHeight)
    {
        m_malus.fill(PATH_MALUS_UNSET);
    }

    void setMalus(PathType type, f32 malus)
    {
        m_malus[static_cast<size_t>(type)] = malus;
    }

    void setPosition(f64 x, f64 y, f64 z)
    {
        m_posX = x;
        m_posY = y;
        m_posZ = z;
    }

    void setOnGround(bool onGround)
    {
        m_onGround = onGround;
    }

    [[nodiscard]] f32 pathfindingWidth() const override
    {
        return m_width;
    }

    [[nodiscard]] f32 pathfindingHeight() const override
    {
        return m_height;
    }

    [[nodiscard]] f32 pathfindingStepHeight() const override
    {
        return m_stepHeight;
    }

    [[nodiscard]] i32 pathfindingMaxFallDistance() const override
    {
        return 3;
    }

    [[nodiscard]] f32 pathfindingMalus(PathType type) const override
    {
        const f32 stored = m_malus[static_cast<size_t>(type)];
        return stored == PATH_MALUS_UNSET ? getPathMalus(type) : stored;
    }

    [[nodiscard]] AxisAlignedBB pathfindingBoundingBox() const override
    {
        const f32 half = m_width / 2.0f;
        return AxisAlignedBB(static_cast<f32>(m_posX) - half,
            static_cast<f32>(m_posY),
            static_cast<f32>(m_posZ) - half,
            static_cast<f32>(m_posX) + half,
            static_cast<f32>(m_posY) + m_height,
            static_cast<f32>(m_posZ) + half);
    }

    [[nodiscard]] bool pathfindingOnGround() const override
    {
        return m_onGround;
    }

    [[nodiscard]] bool pathfindingInWater() const override
    {
        return false;
    }

    [[nodiscard]] bool pathfindingCanStandOnFluid(const fluid::FluidState* /*fluidState*/) const override
    {
        return false;
    }

private:
    /// 未显式设置的代价值，回退到该类型的默认代价。
    static constexpr f32 PATH_MALUS_UNSET = 12345.0f;

    f32 m_width;
    f32 m_height;
    f32 m_stepHeight;
    f64 m_posX = 0.5;
    f64 m_posY = 0.0;
    f64 m_posZ = 0.5;
    bool m_onGround = true;
    std::array<f32, pathTypeCount()> m_malus{};
};

} // namespace

// ============================================================================
// 单格原始类型
// ============================================================================

TEST(WalkNodeEvaluatorTest, AirIsOpen)
{
    TableRegion region;
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, 0, 0, 0), PathType::Open);
}

TEST(WalkNodeEvaluatorTest, SolidBlockIsBlocked)
{
    TableRegion region;
    region.setBlock(0, 0, 0, VanillaBlocks::STONE->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, 0, 0, 0), PathType::Blocked);
}

TEST(WalkNodeEvaluatorTest, WaterIsWaterAndLavaIsLava)
{
    TableRegion region;
    region.setBlock(0, 0, 0, VanillaBlocks::WATER->defaultState());
    region.setWater(0, 0, 0);
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, 0, 0, 0), PathType::Water);

    region.setBlock(0, 1, 0, VanillaBlocks::LAVA->defaultState());
    region.setLava(0, 1, 0);
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, 0, 1, 0), PathType::Lava);
}

TEST(WalkNodeEvaluatorTest, HazardBlocksMapToDangerTypes)
{
    TableRegion region;
    region.setBlock(0, 0, 0, VanillaBlocks::FIRE->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, 0, 0, 0), PathType::DamageFire);

    region.setBlock(1, 0, 0, VanillaBlocks::MAGMA->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, 1, 0, 0), PathType::DamageFire);

    region.setBlock(2, 0, 0, VanillaBlocks::CACTUS->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, 2, 0, 0), PathType::DamageOther);

    region.setBlock(3, 0, 0, VanillaBlocks::HONEY_BLOCK->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, 3, 0, 0), PathType::StickyHoney);

    region.setBlock(4, 0, 0, VanillaBlocks::POWDER_SNOW->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, 4, 0, 0), PathType::PowderSnow);
}

TEST(WalkNodeEvaluatorTest, PlantWithoutCollisionIsOpen)
{
    TableRegion region;
    region.setBlock(0, 0, 0, VanillaBlocks::POPPY->defaultState());
    region.setShapeHeight(0, 0, 0, 0.0f);
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeFromState(region, 0, 0, 0), PathType::Open);
}

// ============================================================================
// 静态类型（叠加脚下与邻域）
// ============================================================================

TEST(WalkNodeEvaluatorTest, SolidBelowMakesWalkable)
{
    TableRegion region;
    region.setBlock(0, -1, 0, VanillaBlocks::STONE->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeStatic(region, 0, 0, 0), PathType::Walkable);
}

TEST(WalkNodeEvaluatorTest, AirBelowStaysOpen)
{
    TableRegion region;
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeStatic(region, 0, 0, 0), PathType::Open);
}

TEST(WalkNodeEvaluatorTest, FireAdjacentMakesDangerFire)
{
    TableRegion region;
    // 脚下是实心方块，因而进入 3×3×3 邻域检查；旁边有火 → 整个格子标成危险火区。
    region.setBlock(0, -1, 0, VanillaBlocks::STONE->defaultState());
    region.setBlock(1, 0, 0, VanillaBlocks::FIRE->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeStatic(region, 0, 0, 0), PathType::DangerFire);
}

TEST(WalkNodeEvaluatorTest, WaterAdjacentMakesWaterBorder)
{
    TableRegion region;
    region.setBlock(0, -1, 0, VanillaBlocks::STONE->defaultState());
    region.setBlock(0, 0, 1, VanillaBlocks::WATER->defaultState());
    region.setWater(0, 0, 1);
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeStatic(region, 0, 0, 0), PathType::WaterBorder);
}

TEST(WalkNodeEvaluatorTest, PowderSnowBelowMakesDangerPowderSnow)
{
    TableRegion region;
    region.setBlock(0, -1, 0, VanillaBlocks::POWDER_SNOW->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeStatic(region, 0, 0, 0), PathType::DangerPowderSnow);
}

TEST(WalkNodeEvaluatorTest, HazardNeighbourPriorityPrefersOtherOverFire)
{
    TableRegion region;
    region.setBlock(0, -1, 0, VanillaBlocks::STONE->defaultState());
    // 邻域里同时有仙人掌与火：原版按 dx,dy,dz 的循环顺序先遇到仙人掌 → DANGER_OTHER。
    region.setBlock(-1, -1, 0, VanillaBlocks::CACTUS->defaultState());
    region.setBlock(1, 0, 1, VanillaBlocks::FIRE->defaultState());
    EXPECT_EQ(WalkNodeEvaluator::getPathTypeStatic(region, 0, 0, 0), PathType::DangerOther);
}

// ============================================================================
// 含实体尺寸的类型
// ============================================================================

namespace {

/// 搭一个「脚下实心、当前格空气」的三格高空间，便于测实体尺寸相关规则。
void buildFlatFloor(TableRegion& region, i32 radius)
{
    for (i32 x = -radius; x <= radius; ++x) {
        for (i32 z = -radius; z <= radius; ++z) {
            region.setBlock(x, -1, z, VanillaBlocks::STONE->defaultState());
        }
    }
}

} // namespace

TEST(WalkNodeEvaluatorTest, StandingOnFloorIsWalkable)
{
    TableRegion region;
    buildFlatFloor(region, 2);
    FakeEntity entity(0.6f, 1.8f);
    entity.setPosition(0.5, 0.0, 0.5);
    const PathfindingContext context(region, entity, Vector3d(0.5, 0.0, 0.5));
    WalkNodeEvaluator evaluator;
    evaluator.prepare(region, entity, Vector3d(0.5, 0.0, 0.5));
    EXPECT_EQ(evaluator.getPathTypeOfMob(context, 0, 0, 0), PathType::Walkable);
}

TEST(WalkNodeEvaluatorTest, NegativeMalusTypeIsReturned)
{
    TableRegion region;
    buildFlatFloor(region, 2);
    region.setBlock(1, 0, 0, VanillaBlocks::WATER->defaultState());
    region.setWater(1, 0, 0);

    FakeEntity entity(0.6f, 1.8f);
    entity.setPosition(0.5, 0.0, 0.5);
    entity.setMalus(PathType::Water, -1.0f);

    const PathfindingContext context(region, entity, Vector3d(0.5, 0.0, 0.5));
    WalkNodeEvaluator evaluator;
    evaluator.prepare(region, entity, Vector3d(0.5, 0.0, 0.5));
    EXPECT_EQ(evaluator.getPathTypeOfMob(context, 1, 0, 0), PathType::Water);
}

TEST(WalkNodeEvaluatorTest, ClosedWoodenDoorIsWalkableWhenEntityCanOpenDoors)
{
    TableRegion region;
    buildFlatFloor(region, 2);
    // 门就放在实体所在的格子上：判定要看「实体包围盒覆盖的格子」，
    // 检查邻格会被实体自己脚下那格的可走性掩盖。
    region.setBlock(0, 0, 0, VanillaBlocks::OAK_DOOR->defaultState());
    region.setShapeHeight(0, 0, 0, 0.0f);

    FakeEntity entity(0.6f, 1.8f);
    entity.setPosition(0.5, 0.0, 0.5);
    const PathfindingContext context(region, entity, Vector3d(0.5, 0.0, 0.5));

    WalkNodeEvaluator evaluator;
    evaluator.prepare(region, entity, Vector3d(0.5, 0.0, 0.5));
    evaluator.setCanOpenDoors(true);
    evaluator.setCanPassDoors(true);
    // 搜索开始时门还是关着的：能开门且能穿门的实体把它当作「可穿过的门」。
    EXPECT_EQ(evaluator.getPathTypeOfMob(context, 0, 0, 0), PathType::WalkableDoor);
}

TEST(WalkNodeEvaluatorTest, ClosedWoodenDoorBlocksEntityThatCannotOpenDoors)
{
    TableRegion region;
    buildFlatFloor(region, 2);
    region.setBlock(0, 0, 0, VanillaBlocks::OAK_DOOR->defaultState());
    region.setShapeHeight(0, 0, 0, 0.0f);

    FakeEntity entity(0.6f, 1.8f);
    entity.setPosition(0.5, 0.0, 0.5);
    const PathfindingContext context(region, entity, Vector3d(0.5, 0.0, 0.5));

    WalkNodeEvaluator evaluator;
    evaluator.prepare(region, entity, Vector3d(0.5, 0.0, 0.5));
    evaluator.setCanOpenDoors(false);
    evaluator.setCanPassDoors(true);
    // 不会开门的实体面前，木门就是不可通行的实体方块。
    EXPECT_EQ(evaluator.getPathTypeOfMob(context, 0, 0, 0), PathType::DoorWoodClosed);
}

TEST(WalkNodeEvaluatorTest, RailNotAlignedIsUnpassable)
{
    TableRegion region;
    buildFlatFloor(region, 2);
    region.setBlock(1, 0, 0, VanillaBlocks::RAIL->defaultState());
    region.setShapeHeight(1, 0, 0, 0.0f);

    FakeEntity entity(0.6f, 1.8f);
    entity.setPosition(0.5, 0.0, 0.5);
    const PathfindingContext context(region, entity, Vector3d(0.5, 0.0, 0.5));
    WalkNodeEvaluator evaluator;
    evaluator.prepare(region, entity, Vector3d(0.5, 0.0, 0.5));
    // 实体不在铁轨上、脚下也不是铁轨：远处的铁轨不可上下。
    EXPECT_EQ(evaluator.getPathTypeOfMob(context, 1, 0, 0), PathType::UnpassableRail);
}

TEST(WalkNodeEvaluatorTest, WideEntitySeesFenceInAdjacentCell)
{
    TableRegion region;
    buildFlatFloor(region, 2);
    // 栅栏放在东侧相邻格。
    region.setBlock(1, 0, 0, VanillaBlocks::OAK_FENCE->defaultState());

    // 宽度 1.4 格、中心在 x = 0.7：包围盒覆盖 x ∈ [0.0, 1.4]，因此相邻格的栅栏会被算进来。
    // 实体占几格由 `prepare` 算出（floor(1.4 + 1) = 2），必须先开会话再查询。
    FakeEntity entity(1.4f, 1.8f);
    entity.setPosition(0.7, 0.0, 0.5);
    WalkNodeEvaluator evaluator;
    evaluator.prepare(region, entity, Vector3d(0.7, 0.0, 0.5));
    EXPECT_EQ(evaluator.getPathTypeOfMob(*evaluator.context(), 0, 0, 0), PathType::Fence);
    evaluator.done();
}

TEST(WalkNodeEvaluatorTest, NarrowEntityDoesNotSeeFenceInAdjacentCell)
{
    TableRegion region;
    buildFlatFloor(region, 2);
    region.setBlock(1, 0, 0, VanillaBlocks::OAK_FENCE->defaultState());

    // 宽度 0.6 格、中心在 x = 0.5：包围盒只覆盖 x ∈ [0.2, 0.8]，看不到相邻格的栅栏。
    FakeEntity entity(0.6f, 1.8f);
    entity.setPosition(0.5, 0.0, 0.5);
    const PathfindingContext context(region, entity, Vector3d(0.5, 0.0, 0.5));
    WalkNodeEvaluator evaluator;
    evaluator.prepare(region, entity, Vector3d(0.5, 0.0, 0.5));
    EXPECT_EQ(evaluator.getPathTypeOfMob(context, 0, 0, 0), PathType::Walkable);
}

TEST(WalkNodeEvaluatorTest, PathTypeCountMatchesVanilla)
{
    // 1.21.11 的 PathType 一共 26 项（含 POWDER_SNOW / DANGER_POWDER_SNOW / DAMAGE_CAUTIOUS 等）。
    EXPECT_EQ(pathTypeCount(), 26u);
    EXPECT_LT(getPathMalus(PathType::Lava), 0.0f);
    EXPECT_GT(getPathMalus(PathType::Water), 0.0f);
    EXPECT_EQ(getPathMalus(PathType::Open), 0.0f);
}
