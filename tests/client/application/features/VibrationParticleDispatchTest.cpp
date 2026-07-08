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
 * @file VibrationParticleDispatchTest.cpp
 * @brief 振动粒子目标位置解析纯函数单元测试
 *
 * 验证 ClientApplicationNetwork 的振动粒子回调中"根据目标来源类型解析目标坐标"
 * 的纯函数 resolveVibrationTargetPosition 的行为。该函数对应 MC Java
 * VibrationSignalParticle 接收时一次性解析 PositionSource.getPosition 的逻辑。
 *
 * 覆盖：
 * - 方块来源（kind=0）：直接返回解码坐标
 * - 实体来源（kind=1）+ 实体存在：返回实体位置叠加 yOffset
 * - 实体来源（kind=1）+ 实体不存在：返回 nullopt（粒子放弃生成）
 * - 实体来源（kind=1）+ INVALID_ENTITY_ID：返回 nullopt
 * - 实体来源（kind=1）+ 空 entityLookup 回调：返回 nullopt
 * - 未知来源类型：回退为方块来源语义
 * - yOffset 正负值
 * - 坐标精度（f64 传递）
 *
 * 测试采用 mock 实体查找回调，避免依赖 ClientEntityManager。
 */

#include "client/application/features/VibrationTargetResolver.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <unordered_map>

#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"

using namespace mc;
using namespace mc::client;
using namespace mc::client::application::features;

namespace {

/// @brief 测试夹具：管理一组 mock ClientEntity 供 entityLookup 回调使用
class VibrationParticleDispatchTest : public ::testing::Test {
protected:
    void TearDown() override { m_targets.clear(); }

    /// @brief 创建一个目标 ClientEntity 并返回其 EntityId
    EntityId makeTargetEntity(f32 x, f32 y, f32 z)
    {
        EntityId id = m_nextId;
        m_nextId = EntityId(static_cast<u32>(m_nextId) + 1);
        auto target = std::make_unique<ClientEntity>(id, "minecraft:player");
        target->setPosition(x, y, z);
        m_targets[id] = std::move(target);
        return id;
    }

    /// @brief 实体查找回调，模拟 ClientEntityManager::getEntity
    std::function<const ClientEntity*(EntityId)> makeLookup()
    {
        return [this](EntityId id) -> const ClientEntity* {
            auto it = m_targets.find(id);
            if (it == m_targets.end()) {
                return nullptr;
            }
            return it->second.get();
        };
    }

    std::unordered_map<EntityId, std::unique_ptr<ClientEntity>> m_targets;
    EntityId m_nextId = EntityId(100);
};

} // namespace

// ============================================================================
// 方块来源（kind=0）
// ============================================================================

TEST_F(VibrationParticleDispatchTest, BlockSource_ReturnsDecodedPositionDirectly)
{
    auto lookup = makeLookup();
    auto result = resolveVibrationTargetPosition(0, 1.5, 64.0, -2.5, INVALID_ENTITY_ID, 0.0f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 1.5);
    EXPECT_DOUBLE_EQ(result->y, 64.0);
    EXPECT_DOUBLE_EQ(result->z, -2.5);
}

TEST_F(VibrationParticleDispatchTest, BlockSource_IgnoresEntityFields)
{
    // 即使传入了 targetEntityId/yOffset，方块来源也直接使用 targetX/Y/Z
    auto lookup = makeLookup();
    const EntityId someId = makeTargetEntity(100.0f, 200.0f, 300.0f);
    auto result = resolveVibrationTargetPosition(0, 10.0, 20.0, 30.0, someId, 1.5f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 10.0);
    EXPECT_DOUBLE_EQ(result->y, 20.0);
    EXPECT_DOUBLE_EQ(result->z, 30.0);
}

TEST_F(VibrationParticleDispatchTest, BlockSource_NegativeCoordinates)
{
    auto lookup = makeLookup();
    auto result = resolveVibrationTargetPosition(0, -100000.5, -64.0, -200000.25, INVALID_ENTITY_ID, 0.0f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, -100000.5);
    EXPECT_DOUBLE_EQ(result->y, -64.0);
    EXPECT_DOUBLE_EQ(result->z, -200000.25);
}

// ============================================================================
// 实体来源（kind=1）+ 实体存在
// ============================================================================

TEST_F(VibrationParticleDispatchTest, EntitySource_Found_ReturnsEntityPositionPlusYOffset)
{
    auto lookup = makeLookup();
    const EntityId id = makeTargetEntity(10.5f, 64.0f, -20.0f);
    auto result = resolveVibrationTargetPosition(1, 0.0, 0.0, 0.0, id, 1.62f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 10.5);
    // yOffset 是 f32，1.62f 提升为 f64 后为 1.6200000047683716，因此 y 按 f32 精度比较
    EXPECT_FLOAT_EQ(static_cast<f32>(result->y), 64.0f + 1.62f);
    EXPECT_DOUBLE_EQ(result->z, -20.0);
}

TEST_F(VibrationParticleDispatchTest, EntitySource_Found_ZeroYOffset)
{
    auto lookup = makeLookup();
    const EntityId id = makeTargetEntity(0.0f, 0.0f, 0.0f);
    auto result = resolveVibrationTargetPosition(1, 5.0, 5.0, 5.0, id, 0.0f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 0.0);
    EXPECT_DOUBLE_EQ(result->y, 0.0);
    EXPECT_DOUBLE_EQ(result->z, 0.0);
}

TEST_F(VibrationParticleDispatchTest, EntitySource_Found_NegativeYOffset)
{
    auto lookup = makeLookup();
    const EntityId id = makeTargetEntity(50.0f, 100.0f, 50.0f);
    auto result = resolveVibrationTargetPosition(1, 0.0, 0.0, 0.0, id, -2.5f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 50.0);
    EXPECT_DOUBLE_EQ(result->y, 100.0 - 2.5);
    EXPECT_DOUBLE_EQ(result->z, 50.0);
}

TEST_F(VibrationParticleDispatchTest, EntitySource_Found_PreservesEntityPositionChanges)
{
    // 验证 entityLookup 回调返回的是实体当前最新位置
    auto lookup = makeLookup();
    const EntityId id = makeTargetEntity(1.0f, 2.0f, 3.0f);
    m_targets[id]->setPosition(100.0f, 200.0f, 300.0f);

    auto result = resolveVibrationTargetPosition(1, 0.0, 0.0, 0.0, id, 0.0f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 100.0);
    EXPECT_DOUBLE_EQ(result->y, 200.0);
    EXPECT_DOUBLE_EQ(result->z, 300.0);
}

// ============================================================================
// 实体来源（kind=1）+ 实体不存在
// ============================================================================

TEST_F(VibrationParticleDispatchTest, EntitySource_NotFound_ReturnsNullopt)
{
    auto lookup = makeLookup();
    // 没有创建任何实体，直接查找未知的 EntityId
    auto result = resolveVibrationTargetPosition(1, 0.0, 0.0, 0.0, EntityId(999), 1.0f, lookup);
    EXPECT_FALSE(result.has_value());
}

TEST_F(VibrationParticleDispatchTest, EntitySource_InvalidEntityId_ReturnsNullopt)
{
    auto lookup = makeLookup();
    auto result = resolveVibrationTargetPosition(1, 0.0, 0.0, 0.0, INVALID_ENTITY_ID, 1.0f, lookup);
    EXPECT_FALSE(result.has_value());
}

TEST_F(VibrationParticleDispatchTest, EntitySource_NullLookupCallback_ReturnsNullopt)
{
    // 空 entityLookup 回调（模拟 ClientEntityManager 未初始化或异常场景）
    std::function<const ClientEntity*(EntityId)> emptyLookup;
    auto result = resolveVibrationTargetPosition(1, 0.0, 0.0, 0.0, EntityId(1), 1.0f, emptyLookup);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// 未知来源类型
// ============================================================================

TEST_F(VibrationParticleDispatchTest, UnknownSourceKind_FallsBackToBlockSemantics)
{
    auto lookup = makeLookup();
    // kind=2 是未定义的来源类型，应回退为方块来源语义（直接使用 targetX/Y/Z）
    auto result = resolveVibrationTargetPosition(2, 7.5, 8.5, 9.5, EntityId(1), 1.0f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 7.5);
    EXPECT_DOUBLE_EQ(result->y, 8.5);
    EXPECT_DOUBLE_EQ(result->z, 9.5);
}

TEST_F(VibrationParticleDispatchTest, HighSourceKind_FallsBackToBlockSemantics)
{
    auto lookup = makeLookup();
    auto result = resolveVibrationTargetPosition(255, 1.0, 2.0, 3.0, INVALID_ENTITY_ID, 0.0f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, 1.0);
    EXPECT_DOUBLE_EQ(result->y, 2.0);
    EXPECT_DOUBLE_EQ(result->z, 3.0);
}

// ============================================================================
// 坐标精度
// ============================================================================

TEST_F(VibrationParticleDispatchTest, BlockSource_PreciseDoubleCoordinates)
{
    // 验证 f64 精度无损传递
    auto lookup = makeLookup();
    const f64 px = 123456.789012345;
    const f64 py = -987654.321098765;
    const f64 pz = 0.000000001;
    auto result = resolveVibrationTargetPosition(0, px, py, pz, INVALID_ENTITY_ID, 0.0f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->x, px);
    EXPECT_DOUBLE_EQ(result->y, py);
    EXPECT_DOUBLE_EQ(result->z, pz);
}

TEST_F(VibrationParticleDispatchTest, EntitySource_PreciseFloatCoordinates)
{
    // ClientEntity 位置是 f32，解析后提升为 f64
    auto lookup = makeLookup();
    const f32 ex = 1.2345678f;
    const f32 ey = 64.5f;
    const f32 ez = -7.89f;
    const EntityId id = makeTargetEntity(ex, ey, ez);
    auto result = resolveVibrationTargetPosition(1, 0.0, 0.0, 0.0, id, 0.0f, lookup);
    ASSERT_TRUE(result.has_value());
    EXPECT_FLOAT_EQ(static_cast<f32>(result->x), ex);
    EXPECT_FLOAT_EQ(static_cast<f32>(result->y), ey);
    EXPECT_FLOAT_EQ(static_cast<f32>(result->z), ez);
}
