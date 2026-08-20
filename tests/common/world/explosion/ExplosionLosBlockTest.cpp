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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/ray/Ray.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionMode.hpp"

#include <cmath>

using namespace mc;
using namespace mc::world::explosion;

namespace mc {
namespace {

/// 带区块存储的爆炸视线测试世界：在 BaseChunkBackedTestWorld 基础上补一个真正从区块读
/// 方块的 getBlockState（基类 BaseTestWorld::getBlockState 恒返回 nullptr），并补
/// setBlockState 写入对应区块，使爆炸专用视线 DDA 与参考 raycastBlocks 都能读到方块。
class ExplosionLosWorld final : public mc::test::BaseChunkBackedTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT || x < 0 || x >= world::CHUNK_WIDTH || z < 0 ||
            z >= world::CHUNK_WIDTH) {
            return nullptr;
        }
        const ChunkData* chunk = getChunk(x >> 4, z >> 4);
        if (chunk == nullptr) {
            return nullptr;
        }
        return chunk->getBlockState(x & 15, y, z & 15);
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
            return false;
        }
        ChunkData& chunk = ensureChunk(x >> 4, z >> 4);
        chunk.setBlockState(x & 15, y, z & 15, state);
        return true;
    }
};

/// 纯桩世界：getChunk 恒返回 nullptr，getBlockState 恒返回 nullptr（全空气）。
/// BaseTestWorld 构造为 protected，经此派生类暴露可构造入口，用于验证专用 DDA 在
/// getChunk 返回 nullptr 时的回退路径（回退到 world.getBlockState）。
class NullChunkStub final : public mc::test::BaseTestWorld {
public:
    NullChunkStub() = default;
};

/// 参考实现：用公共 raycastBlocks 复刻原 _getBlockDensity 的采样逻辑，作为专用 DDA 的
/// 行为等价基准。采样步长/偏移/射线构造与 Explosion::_getBlockDensity 完全一致。
[[nodiscard]] f32 referenceDensityUsingRaycast(
    const IWorld& world, const Vector3& center, const AxisAlignedBB& entityBox)
{
    f32 dx = (entityBox.maxX - entityBox.minX) * 2.0f + 1.0f;
    f32 dy = (entityBox.maxY - entityBox.minY) * 2.0f + 1.0f;
    f32 dz = (entityBox.maxZ - entityBox.minZ) * 2.0f + 1.0f;
    f32 stepX = 1.0f / dx;
    f32 stepY = 1.0f / dy;
    f32 stepZ = 1.0f / dz;
    f32 offsetX = (1.0f - std::floor(1.0f / stepX) * stepX) * 0.5f;
    f32 offsetZ = (1.0f - std::floor(1.0f / stepZ) * stepZ) * 0.5f;
    if (stepX <= 0.0f || stepY <= 0.0f || stepZ <= 0.0f) {
        return 0.0f;
    }

    i32 visible = 0;
    i32 total = 0;
    for (f32 fx = 0.0f; fx <= 1.0f; fx += stepX) {
        for (f32 fy = 0.0f; fy <= 1.0f; fy += stepY) {
            for (f32 fz = 0.0f; fz <= 1.0f; fz += stepZ) {
                Vector3 samplePoint(entityBox.minX + fx * (entityBox.maxX - entityBox.minX) + offsetX,
                    entityBox.minY + fy * (entityBox.maxY - entityBox.minY),
                    entityBox.minZ + fz * (entityBox.maxZ - entityBox.minZ) + offsetZ);
                Ray ray(
                    samplePoint, Vector3(center.x - samplePoint.x, center.y - samplePoint.y, center.z - samplePoint.z));
                f32 distance = (center - samplePoint).length();
                RaycastContext context(ray, distance);
                BlockRaycastResult result = raycastBlocks(context, world);
                if (result.isMiss()) {
                    ++visible;
                }
                ++total;
            }
        }
    }
    return total > 0 ? static_cast<f32>(visible) / static_cast<f32>(total) : 0.0f;
}

} // anonymous namespace

// fixture 放 mc 命名空间非匿名，使 Explosion.hpp 内 friend class ::mc::ExplosionLosBlockTest
// 声明能跨翻译单元匹配（匿名命名空间类具有内部链接，无法跨 TU 被 friend 命名）。
class ExplosionLosBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        m_center = Vector3(0.5f, 64.0f, 0.5f);
    }

    /// 在爆炸中心与实体之间填充一堵墙（指定 Y 列、X 范围），完全遮挡视线
    void buildWall(ExplosionLosWorld& world, const BlockState& state, i32 x0, i32 x1, i32 y0, i32 y1, i32 z)
    {
        for (i32 x = x0; x <= x1; ++x) {
            for (i32 y = y0; y <= y1; ++y) {
                world.setBlockState(x, y, z, &state);
            }
        }
    }

    /// 经 friend 授权访问 Explosion::_getBlockDensity。TEST_F 宏生成的派生测试类不继承
    /// friend 关系，故 private 访问必须经此 fixture 成员函数中转。
    [[nodiscard]] f32 getBlockDensity(Explosion& explosion, const AxisAlignedBB& box)
    {
        return explosion._getBlockDensity(box);
    }

    ExplosionLosWorld m_world;
    Vector3 m_center{0.5f, 64.0f, 0.5f};
};

// ============================================================================
// 场景 A：空世界无遮挡，density 应为 1.0，且与参考实现一致
// ============================================================================

TEST_F(ExplosionLosBlockTest, EmptyWorld_FullVisibility_MatchesRaycast)
{
    // 标准生物碰撞箱 0.6×1.8×0.6，置于爆炸中心附近
    const AxisAlignedBB box(2.0f, 64.0f, 2.0f, 2.6f, 65.8f, 2.6f);
    Explosion explosion(m_world, m_center, 4.0f, ExplosionMode::None);

    const f32 density = getBlockDensity(explosion, box);
    const f32 reference = referenceDensityUsingRaycast(m_world, m_center, box);

    // 空世界所有射线 miss，density 应为 1.0
    EXPECT_FLOAT_EQ(density, 1.0f);
    // 专用 DDA 必须与公共 raycastBlocks 行为等价
    EXPECT_FLOAT_EQ(density, reference);
}

// ============================================================================
// 场景 B：爆炸中心与实体之间填满石头墙，完全遮挡，density 应为 0.0
// ============================================================================

TEST_F(ExplosionLosBlockTest, SolidWall_FullyBlocked_MatchesRaycast)
{
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    // 在 X=1 处建一堵满墙（覆盖实体碰撞箱 Y 范围与射线穿过的 X 列）
    buildWall(m_world, stone, 1, 1, 63, 66, 1);
    buildWall(m_world, stone, 1, 1, 63, 66, 2);

    const AxisAlignedBB box(2.0f, 64.0f, 2.0f, 2.6f, 65.8f, 2.6f);
    Explosion explosion(m_world, m_center, 4.0f, ExplosionMode::None);

    const f32 density = getBlockDensity(explosion, box);
    const f32 reference = referenceDensityUsingRaycast(m_world, m_center, box);

    EXPECT_FLOAT_EQ(density, 0.0f);
    EXPECT_FLOAT_EQ(density, reference);
}

// ============================================================================
// 场景 C：部分遮挡（仅遮挡部分采样点），density 在 (0,1) 且与参考一致
// ============================================================================

TEST_F(ExplosionLosBlockTest, PartialWall_PartiallyBlocked_MatchesRaycast)
{
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    // 在射线穿过 X=1 平面时 Y 区间的上半部分建墙（Y=65 占 [65,66)），仅遮挡 Y 较高的
    // 采样射线；Y 较低的采样射线穿过 X=1 时 Y<65 能从墙下方漏过，形成部分遮挡。
    // 几何：采样点 Y∈[64,65.8]，中心 Y=64，射线穿 X=1（t≈0.6）时 Y≈0.4*采样点Y+38.4，
    // 采样点 Y=65.8 时 Y_穿过≈64.72（漏），Y=65.0 时 Y_穿过≈64.4（漏）——为获得稳定部分
    // 遮挡，墙需覆盖射线实际穿过的 Y 区间，故改用更宽 box 使部分采样点穿 X=1 时 Y≥65。
    buildWall(m_world, stone, 1, 1, 65, 65, 1);
    buildWall(m_world, stone, 1, 1, 65, 65, 2);

    // 加高 box Y 范围使高 Y 采样射线穿 X=1 时落到墙 [65,66) 内被遮挡，
    // 低 Y 采样射线穿 X=1 时 Y<65 漏过
    const AxisAlignedBB box(2.0f, 64.0f, 2.0f, 2.6f, 68.0f, 2.6f);
    Explosion explosion(m_world, m_center, 4.0f, ExplosionMode::None);

    const f32 density = getBlockDensity(explosion, box);
    const f32 reference = referenceDensityUsingRaycast(m_world, m_center, box);

    // 部分遮挡：0 < density < 1
    EXPECT_GT(density, 0.0f);
    EXPECT_LT(density, 1.0f);
    // 关键：专用 DDA 与参考实现逐采样点等价，density 必须精确相等
    EXPECT_FLOAT_EQ(density, reference);
}

// ============================================================================
// 场景 D：玻璃遮挡（非空气非液体，shape 非空），水不遮挡（isLiquid=true）
// ============================================================================

TEST_F(ExplosionLosBlockTest, GlassBlocks_LiquidDoesNot_MatchesRaycast)
{
    // 玻璃应遮挡视线（isAir=false, isLiquid=false, shape 为完整立方体）
    if (VanillaBlocks::GLASS != nullptr) {
        const BlockState& glass = VanillaBlocks::GLASS->defaultState();
        buildWall(m_world, glass, 1, 1, 63, 66, 1);
        buildWall(m_world, glass, 1, 1, 63, 66, 2);

        const AxisAlignedBB box(2.0f, 64.0f, 2.0f, 2.6f, 65.8f, 2.6f);
        Explosion explosion(m_world, m_center, 4.0f, ExplosionMode::None);

        const f32 density = getBlockDensity(explosion, box);
        const f32 reference = referenceDensityUsingRaycast(m_world, m_center, box);
        EXPECT_FLOAT_EQ(density, reference);
    }

    // 水不应遮挡（isLiquid=true，raycast 与专用 DDA 都跳过液体）
    if (VanillaBlocks::WATER != nullptr) {
        // 重建干净世界：清除玻璃墙
        ExplosionLosWorld waterWorld;
        const BlockState& water = VanillaBlocks::WATER->defaultState();
        buildWall(waterWorld, water, 1, 1, 63, 66, 1);
        buildWall(waterWorld, water, 1, 1, 63, 66, 2);

        const AxisAlignedBB box(2.0f, 64.0f, 2.0f, 2.6f, 65.8f, 2.6f);
        Explosion explosion(waterWorld, m_center, 4.0f, ExplosionMode::None);

        const f32 density = getBlockDensity(explosion, box);
        const f32 reference = referenceDensityUsingRaycast(waterWorld, m_center, box);
        // 水不遮挡，density 应为 1.0
        EXPECT_FLOAT_EQ(density, 1.0f);
        EXPECT_FLOAT_EQ(density, reference);
    }
}

// ============================================================================
// 场景 E：实体紧贴爆炸中心（distance 极小），验证零向量/重合边界不崩溃
// ============================================================================

TEST_F(ExplosionLosBlockTest, EntityAtCenter_NoCrash_MatchesRaycast)
{
    // 实体碰撞箱包含爆炸中心，部分采样点与中心重合（零向量分支）
    const AxisAlignedBB box(0.2f, 63.0f, 0.2f, 1.0f, 65.0f, 1.0f);
    Explosion explosion(m_world, m_center, 4.0f, ExplosionMode::None);

    const f32 density = getBlockDensity(explosion, box);
    const f32 reference = referenceDensityUsingRaycast(m_world, m_center, box);

    EXPECT_FLOAT_EQ(density, reference);
    // 空世界，无遮挡
    EXPECT_FLOAT_EQ(density, 1.0f);
}

// ============================================================================
// 场景 F：getChunk 恒返回 nullptr 的桩世界回退路径，与参考一致
// 验证测试桩世界兼容性：getChunk 返回 nullptr 时专用 DDA 回退 world.getBlockState
// ============================================================================

TEST_F(ExplosionLosBlockTest, NullChunkWorld_Fallback_MatchesRaycast)
{
    // 纯桩世界：getChunk 恒返回 nullptr，getBlockState 恒返回 nullptr（全空气）。
    // BaseTestWorld 构造为 protected，经 NullChunkStub 派生可构造。
    NullChunkStub nullWorld;
    const AxisAlignedBB box(2.0f, 64.0f, 2.0f, 2.6f, 65.8f, 2.6f);
    Explosion explosion(nullWorld, m_center, 4.0f, ExplosionMode::None);

    const f32 density = getBlockDensity(explosion, box);
    const f32 reference = referenceDensityUsingRaycast(nullWorld, m_center, box);

    // 全空气，density 应为 1.0
    EXPECT_FLOAT_EQ(density, 1.0f);
    EXPECT_FLOAT_EQ(density, reference);
}

} // namespace mc
