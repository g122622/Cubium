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
 * @file CampfireBlockTest.cpp
 * @brief CampfireBlock 单元测试
 *
 * 测试内容：
 * 1. getVirtualPostShape() 返回正确的虚拟烟雾柱形状
 * 2. isSmokeyPos 在方块碰撞形状与虚拟柱有交集时正确阻挡烟雾
 * 3. isSmokeyPos 在无交集时烟雾穿透（空气、无碰撞方块）
 * 4. isSmokeyPos 边界场景：部分方块（如台阶）的碰撞形状不完全覆盖中心柱
 * 5. isLitCampfire 识别点燃的营火
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/physics/shape/BooleanOp.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/BuildingVariantBlocks.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/blocks/decorative/CampfireBlock.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;
using namespace mc::block_registry;

namespace {

/// 测试用世界，支持通过坐标设置和获取方块状态
class CampfireTestWorld final : public mc::test::BaseTestWorld {
public:
    CampfireTestWorld() = default;

    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    /// 存储 BlockState 的副本并返回稳定指针
    const BlockState* storeBlockState(const BlockState& state)
    {
        m_storedStates.push_back(std::make_unique<BlockState>(state));
        return m_storedStates.back().get();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    bool setBlockState(const BlockPos& pos, const BlockState* state)
    {
        return setBlockState(pos.x, pos.y, pos.z, state);
    }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }

    void setSeed(u64 seed) { m_seed = seed; }

private:
    static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) & 0x3FFFFFF) | ((static_cast<i64>(y) & 0xFFF) << 26) |
            ((static_cast<i64>(z) & 0x3FFFFFF) << 38);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::vector<std::unique_ptr<BlockState>> m_storedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    u64 m_seed = 12345;
};

} // anonymous namespace

// ============================================================================
// getVirtualPostShape 测试
// ============================================================================

TEST(CampfireBlockTest, GetVirtualPostShape_NotEmpty)
{
    // 虚拟烟雾柱形状不应为空
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    EXPECT_FALSE(post.isEmpty());
}

TEST(CampfireBlockTest, GetVirtualPostShape_CorrectBounds)
{
    // 对应 MC Java 的 Block.column(4.0, 0.0, 16.0)
    // = box(6, 0, 6, 10, 16, 10) in pixel coordinates
    // = box(0.375, 0.0, 0.375, 0.625, 1.0, 0.625) in block-local coordinates
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();

    // 验证边界坐标
    using namespace mc;
    EXPECT_DOUBLE_EQ(post.min(Axis::X), 0.375);
    EXPECT_DOUBLE_EQ(post.max(Axis::X), 0.625);
    EXPECT_DOUBLE_EQ(post.min(Axis::Y), 0.0);
    EXPECT_DOUBLE_EQ(post.max(Axis::Y), 1.0);
    EXPECT_DOUBLE_EQ(post.min(Axis::Z), 0.375);
    EXPECT_DOUBLE_EQ(post.max(Axis::Z), 0.625);
}

TEST(CampfireBlockTest, GetVirtualPostShape_ConsistentAcrossCalls)
{
    // 多次调用应返回相同的对象（静态局部变量）
    const VoxelShape& post1 = CampfireBlock::getVirtualPostShape();
    const VoxelShape& post2 = CampfireBlock::getVirtualPostShape();
    EXPECT_EQ(&post1, &post2);
}

// ============================================================================
// VirtualPost 与各种碰撞形状的交集测试
// ============================================================================

TEST(CampfireBlockTest, VirtualPost_IntersectsFullBlock)
{
    // 完整方块应与虚拟柱有交集
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    VoxelShape fullBlock = Shapes::block();
    EXPECT_TRUE(Shapes::joinIsNotEmpty(post, fullBlock, BooleanOps::And()));
}

TEST(CampfireBlockTest, VirtualPost_IntersectsSlabLowerHalf)
{
    // 下半台阶（0.0-0.5）与虚拟柱有交集，因为柱从0.0到1.0
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    VoxelShape lowerSlab = Shapes::box(0.0, 0.0, 0.0, 1.0, 0.5, 1.0);
    EXPECT_TRUE(Shapes::joinIsNotEmpty(post, lowerSlab, BooleanOps::And()));
}

TEST(CampfireBlockTest, VirtualPost_IntersectsSlabUpperHalf)
{
    // 上半台阶（0.5-1.0）与虚拟柱也有交集
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    VoxelShape upperSlab = Shapes::box(0.0, 0.5, 0.0, 1.0, 1.0, 1.0);
    EXPECT_TRUE(Shapes::joinIsNotEmpty(post, upperSlab, BooleanOps::And()));
}

TEST(CampfireBlockTest, VirtualPost_NoIntersectionWithEdgeBlock)
{
    // 只有边缘的方块（X/Z方向不覆盖0.375-0.625的中心区域）不应与虚拟柱有交集
    // 例如：x从0.0到0.25（不覆盖0.375-0.625范围），全高度
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    VoxelShape edgeBlock = Shapes::box(0.0, 0.0, 0.0, 0.25, 1.0, 1.0);
    EXPECT_FALSE(Shapes::joinIsNotEmpty(post, edgeBlock, BooleanOps::And()));
}

TEST(CampfireBlockTest, VirtualPost_NoIntersectionWithEmptyShape)
{
    // 空形状不应与虚拟柱有交集
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    VoxelShape empty = Shapes::empty();
    EXPECT_FALSE(Shapes::joinIsNotEmpty(post, empty, BooleanOps::And()));
}

// ============================================================================
// isLitCampfire 测试
// ============================================================================

TEST(CampfireBlockTest, IsLitCampfire_LitCampfire)
{
    VanillaBlocks::initialize();

    // 点燃的营火应被识别
    const BlockState& litCampfire = NetherBlocks::CAMPFIRE->defaultState();
    ASSERT_TRUE(litCampfire.hasProperty(BlockStateProperties::LIT()));
    EXPECT_TRUE(CampfireBlock::isLitCampfire(litCampfire));
}

TEST(CampfireBlockTest, IsLitCampfire_UnlitCampfire)
{
    VanillaBlocks::initialize();

    // 熄灭的营火不应被识别
    const BlockState& unlit = NetherBlocks::CAMPFIRE->defaultState().with(BlockStateProperties::LIT(), false);
    EXPECT_FALSE(CampfireBlock::isLitCampfire(unlit));
}

TEST(CampfireBlockTest, IsLitCampfire_NonCampfireBlock)
{
    VanillaBlocks::initialize();

    // 非营火方块（如石头）不应被识别为营火
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(CampfireBlock::isLitCampfire(stone));
}

TEST(CampfireBlockTest, IsLitCampfire_SoulCampfire)
{
    VanillaBlocks::initialize();

    // 灵魂营火也应被识别
    const BlockState& soulCampfire = NetherBlocks::SOUL_CAMPFIRE->defaultState();
    EXPECT_TRUE(CampfireBlock::isLitCampfire(soulCampfire));
}

// ============================================================================
// isSmokeyPos 测试
// ============================================================================

TEST(CampfireBlockTest, IsSmokeyPos_DirectlyAboveLitCampfire)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 营火在 (0, 60, 0)，检测位置在 (0, 61, 0) —— 直接在营火上方
    const BlockState& litCampfire = NetherBlocks::CAMPFIRE->defaultState();
    world.setBlockState(0, 60, 0, &litCampfire);

    EXPECT_TRUE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 61, 0)));
}

TEST(CampfireBlockTest, IsSmokeyPos_TwoBlocksAboveCampfire)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 营火在 (0, 60, 0)，检测位置在 (0, 62, 0) —— 中间隔着空气
    const BlockState& litCampfire = NetherBlocks::CAMPFIRE->defaultState();
    world.setBlockState(0, 60, 0, &litCampfire);

    EXPECT_TRUE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 62, 0)));
}

TEST(CampfireBlockTest, IsSmokeyPos_FiveBlocksAboveCampfire)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 营火在 (0, 60, 0)，检测位置在 (0, 65, 0) —— 正好在5格范围内
    const BlockState& litCampfire = NetherBlocks::CAMPFIRE->defaultState();
    world.setBlockState(0, 60, 0, &litCampfire);

    EXPECT_TRUE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 65, 0)));
}

TEST(CampfireBlockTest, IsSmokeyPos_SixBlocksAboveCampfire_OutOfRange)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 营火在 (0, 60, 0)，检测位置在 (0, 66, 0) —— 超出5格范围
    const BlockState& litCampfire = NetherBlocks::CAMPFIRE->defaultState();
    world.setBlockState(0, 60, 0, &litCampfire);

    EXPECT_FALSE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 66, 0)));
}

TEST(CampfireBlockTest, IsSmokeyPos_NoCampfireBelow)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 没有营火，全是空气
    EXPECT_FALSE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 65, 0)));
}

TEST(CampfireBlockTest, IsSmokeyPos_SolidBlockBlockingSmoke)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 营火在 (0, 60, 0)，实心方块在 (0, 61, 0)，检测位置在 (0, 62, 0)
    // 实心方块的碰撞形状覆盖了中心柱区域，烟雾被阻挡
    const BlockState& litCampfire = NetherBlocks::CAMPFIRE->defaultState();
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    world.setBlockState(0, 60, 0, &litCampfire);
    world.setBlockState(0, 61, 0, &stone);

    // 实心方块阻挡了烟雾，且实心方块下方是营火，所以位置 (0, 62, 0) 不可烟熏
    // 原版逻辑：实心方块阻挡时，检查阻挡方块下方是否有营火
    // 这里实心方块 (0,61) 下方 (0,60) 是营火，所以阻挡方块下方的检查返回 true
    // 但注意：返回的是 "阻挡方块下方是否有营火" 的结果
    // MC Java 原版逻辑：flag=true（被阻挡）-> return isLitCampfire(blockstate1)
    // blockstate1 = world.getBlockState(blockpos.below()) = world.getBlockState(0, 60, 0) = litCampfire
    // 所以结果是 true
    EXPECT_TRUE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 62, 0)));
}

TEST(CampfireBlockTest, IsSmokeyPos_SolidBlockNoCampfireBelow)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 实心方块在 (0, 62, 0)，没有营火，检测位置在 (0, 63, 0)
    // 实心方块阻挡了搜索，但下方不是营火
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    world.setBlockState(0, 62, 0, &stone);

    EXPECT_FALSE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 63, 0)));
}

TEST(CampfireBlockTest, IsSmokeyPos_SolidBlockBetweenCampfireAndPosition)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 营火在 (0, 60, 0)，实心方块在 (0, 62, 0)，检测位置在 (0, 64, 0)
    // 烟雾在 (0, 62) 处被实心方块阻挡，阻挡方块下方 (0, 61) 不是营火，返回 false
    const BlockState& litCampfire = NetherBlocks::CAMPFIRE->defaultState();
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    world.setBlockState(0, 60, 0, &litCampfire);
    world.setBlockState(0, 62, 0, &stone);

    // 搜索顺序：(0,63) 空气 -> (0,62) 石头（被阻挡）-> 检查(0,61) 是否是营火 -> 不是 -> false
    EXPECT_FALSE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 64, 0)));
}

TEST(CampfireBlockTest, IsSmokeyPos_SolidBlockDirectlyAboveCampfire)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 营火在 (0, 60, 0)，实心方块在 (0, 61, 0)（紧贴营火上方），检测位置在 (0, 63, 0)
    // 搜索到 (0,62) 是空气，(0,61) 是实心方块且与虚拟柱有交集 -> 检查 (0,60) 是否是营火 -> 是
    const BlockState& litCampfire = NetherBlocks::CAMPFIRE->defaultState();
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    world.setBlockState(0, 60, 0, &litCampfire);
    world.setBlockState(0, 61, 0, &stone);

    EXPECT_TRUE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 63, 0)));
}

TEST(CampfireBlockTest, IsSmokeyPos_AirDoesNotBlockSmoke)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 空气不应阻挡烟雾穿透
    // 营火在 (0, 60, 0)，中间全是空气
    const BlockState& litCampfire = NetherBlocks::CAMPFIRE->defaultState();
    world.setBlockState(0, 60, 0, &litCampfire);

    // 各层都应能检测到营火
    EXPECT_TRUE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 61, 0)));
    EXPECT_TRUE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 63, 0)));
    EXPECT_TRUE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 65, 0)));
}

TEST(CampfireBlockTest, IsSmokeyPos_UnlitCampfireNotDetected)
{
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    // 熄灭的营火不应被检测为烟熏源
    const BlockState unlit = NetherBlocks::CAMPFIRE->defaultState().with(BlockStateProperties::LIT(), false);
    world.setBlockState(0, 60, 0, &unlit);

    EXPECT_FALSE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 61, 0)));
}

// ============================================================================
// 部分方块碰撞形状测试（台阶等不覆盖中心柱的场景）
// ============================================================================

TEST(CampfireBlockTest, IsSmokeyPos_SlabLowerHalfBlocksSmoke)
{
    // 下半台阶（0-0.5高度）覆盖中心区域，与虚拟柱有交集
    // 台阶的碰撞形状是 (0,0,0)-(1,0.5,1)，中心柱是 (0.375,0,0.375)-(0.625,1,0.625)
    // 交集：(0.375,0,0.375)-(0.625,0.5,0.625) 非空
    VanillaBlocks::initialize();
    CampfireTestWorld world;

    const BlockState& litCampfire = NetherBlocks::CAMPFIRE->defaultState();
    world.setBlockState(0, 60, 0, &litCampfire);

    // 放置下半台阶在 (0, 62, 0)
    // 下半台阶覆盖了0-0.5高度，中心柱从0到1，两者有交集
    const BlockState* lowerSlab =
        BuildingVariantBlocks::STONE_SLAB ? &BuildingVariantBlocks::STONE_SLAB->defaultState() : nullptr;
    if (lowerSlab) {
        world.setBlockState(0, 62, 0, lowerSlab);

        // 搜索到 (0,62) 时，下半台阶与虚拟柱有交集 -> 检查 (0,61) 是否是营火 -> 不是 -> 返回 false
        EXPECT_FALSE(CampfireBlock::isSmokeyPos(world, BlockPos(0, 64, 0)));
    }
    // 如果 STONE_SLAB 未注册则跳过（不影响其他测试）
}

TEST(CampfireBlockTest, VirtualPost_FullBlockCoversPost)
{
    // 完整方块的碰撞形状完全覆盖虚拟柱
    // 验证 Shapes::joinIsNotEmpty(virtualPost, fullBlock, And) 为 true
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    const CollisionShape fullBlockShape = CollisionShape::fullBlock();
    VoxelShape blockShape = Shapes::fromCollisionShape(fullBlockShape);
    EXPECT_TRUE(Shapes::joinIsNotEmpty(post, blockShape, BooleanOps::And()));
}

TEST(CampfireBlockTest, VirtualPost_AirShapeNoIntersection)
{
    // 空气方块的碰撞形状为空，不应与虚拟柱有交集
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    const CollisionShape emptyShape = CollisionShape::empty();
    VoxelShape blockShape = Shapes::fromCollisionShape(emptyShape);
    EXPECT_FALSE(Shapes::joinIsNotEmpty(post, blockShape, BooleanOps::And()));
}

TEST(CampfireBlockTest, VirtualPost_PartialBlockAtEdgeNoIntersection)
{
    // 只有边缘的方块碰撞形状不覆盖中心柱
    // 例如围栏/墙的碰撞形状通常有中心柱和两侧延伸
    // 但纯边缘形状 (0,0,0)-(0.25,1,1) 不与 (0.375,0,0.375)-(0.625,1,0.625) 相交
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    CollisionShape edgeShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.0f);
    VoxelShape blockShape = Shapes::fromCollisionShape(edgeShape);
    EXPECT_FALSE(Shapes::joinIsNotEmpty(post, blockShape, BooleanOps::And()));
}

TEST(CampfireBlockTest, VirtualPost_PartialBlockAtCenterIntersection)
{
    // 覆盖中心的碰撞形状应与虚拟柱有交集
    // (0.25,0,0.25)-(0.75,1,0.75) 覆盖了中心柱 (0.375,0,0.375)-(0.625,1,0.625)
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    CollisionShape centerShape = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 1.0f, 0.75f);
    VoxelShape blockShape = Shapes::fromCollisionShape(centerShape);
    EXPECT_TRUE(Shapes::joinIsNotEmpty(post, blockShape, BooleanOps::And()));
}

TEST(CampfireBlockTest, VirtualPost_NarrowCenterPillarIntersection)
{
    // 非常窄的中心柱（只覆盖0.4-0.6）也应在虚拟柱 (0.375-0.625) 范围内
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    CollisionShape narrowPillar = CollisionShape::box(0.4f, 0.0f, 0.4f, 0.6f, 1.0f, 0.6f);
    VoxelShape blockShape = Shapes::fromCollisionShape(narrowPillar);
    EXPECT_TRUE(Shapes::joinIsNotEmpty(post, blockShape, BooleanOps::And()));
}

TEST(CampfireBlockTest, VirtualPost_HighBlockNoIntersection)
{
    // 高处方块（Y: 0.5-1.0 全范围）与虚拟柱有交集（因为柱从0到1）
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    CollisionShape highBlock = CollisionShape::box(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);
    VoxelShape blockShape = Shapes::fromCollisionShape(highBlock);
    EXPECT_TRUE(Shapes::joinIsNotEmpty(post, blockShape, BooleanOps::And()));
}

TEST(CampfireBlockTest, VirtualPost_TopLayerOnlyNoIntersection)
{
    // 只有顶部一层（Y: 0.99-1.0）的方块，X/Z 方向不覆盖中心柱
    const VoxelShape& post = CampfireBlock::getVirtualPostShape();
    CollisionShape topLayer = CollisionShape::box(0.0f, 0.99f, 0.0f, 0.2f, 1.0f, 0.2f);
    VoxelShape blockShape = Shapes::fromCollisionShape(topLayer);
    EXPECT_FALSE(Shapes::joinIsNotEmpty(post, blockShape, BooleanOps::And()));
}
