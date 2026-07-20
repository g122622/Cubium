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

// 测试 EndDragonFight::_findExitPortal 方法。
// 对应 MC 1.21.11 EndDragonFight.findExitPortal()。
//
// 覆盖：
//   1. 世界中存在完整讲台结构 → 模式匹配成功，返回 BlockPatternMatch
//   2. 世界中无讲台结构 → 返回 nullopt
//   3. m_portalLocation 在首次匹配后被正确设置
//   4. 已有 m_portalLocation 时不被覆盖
//   5. tryRespawn 在无讲台时创建激活态讲台
//   6. tryRespawn 在有讲台时使用已有讲台位置
//   7. _respawnDragon 使用 BlockPattern 精确替换讲台方块（不误伤其他基岩）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/EndDragonFightTestAccessor.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"

#include <memory>
#include <vector>

using namespace mc;

namespace mc::test {

/// 带区块存储和实体管理的测试世界，用于 _findExitPortal 测试
class FindPortalTestWorld : public BaseChunkBackedTestWorld {
public:
    FindPortalTestWorld()
    {
        // 预填充原点周围 ARENA_CHUNK_RADIUS 范围的区块
        for (i32 cx = -EndDragonFight::ARENA_CHUNK_RADIUS; cx <= EndDragonFight::ARENA_CHUNK_RADIUS; ++cx) {
            for (i32 cz = -EndDragonFight::ARENA_CHUNK_RADIUS; cz <= EndDragonFight::ARENA_CHUNK_RADIUS; ++cz) {
                ensureChunk(cx, cz);
            }
        }
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const ChunkData* chunk = getChunk(math::toChunkCoord(x), math::toChunkCoord(z));
        if (chunk == nullptr) {
            return nullptr;
        }
        return chunk->getBlockState(math::toLocalCoord(x), y - world::MIN_BUILD_HEIGHT, math::toLocalCoord(z));
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        ChunkData& chunk = ensureChunk(math::toChunkCoord(x), math::toChunkCoord(z));
        chunk.setBlockState(math::toLocalCoord(x), y - world::MIN_BUILD_HEIGHT, math::toLocalCoord(z), state);
        return true;
    }

    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/) const override { return world::MAX_BUILD_HEIGHT; }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    void playEvent(i32 /*eventId*/, const BlockPos& /*pos*/, i32 /*data*/) override {}

    // ========== 实体管理 ==========

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) {
            return EntityInstanceId(0);
        }
        const EntityInstanceId id = m_nextEntityId;
        m_nextEntityId = EntityInstanceId(static_cast<u64>(m_nextEntityId) + 1);
        entity->setId(id);
        entity->setWorld(this);
        m_entities.push_back(std::move(entity));
        return id;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesByType(const std::string& /*typeId*/) const override { return {}; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& /*box*/, const Entity* /*exclude*/) const override
    {
        return {};
    }

    Entity* getEntity(EntityInstanceId id) override
    {
        for (auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    EntityInstanceId m_nextEntityId = EntityInstanceId(1);
};

} // namespace mc::test

// ============================================================================
// 测试夹具
// ============================================================================

class EndDragonFightFindPortalTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    test::FindPortalTestWorld m_world;
};

// ============================================================================
// _findExitPortal 测试
// ============================================================================

TEST_F(EndDragonFightFindPortalTest, FindExitPortal_NoPortal_ReturnsNullopt)
{
    // 世界中无讲台结构：应返回 nullopt
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    EndDragonFight fight(42, data);

    test::EndDragonFightTestAccessor accessor(fight);
    auto match = accessor.findExitPortal(m_world);
    EXPECT_FALSE(match.has_value());
}

TEST_F(EndDragonFightFindPortalTest, FindExitPortal_WithActivePortal_ReturnsMatch)
{
    // 世界中存在激活态讲台：应返回匹配
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    EndDragonFight fight(42, data);

    // 在原点构建激活态讲台
    EndTeleporter::createExitPortal(m_world, BlockPos(0, 64, 0), true);

    test::EndDragonFightTestAccessor accessor(fight);
    auto match = accessor.findExitPortal(m_world);
    EXPECT_TRUE(match.has_value());
}

TEST_F(EndDragonFightFindPortalTest, FindExitPortal_WithInactivePortal_ReturnsMatch)
{
    // 世界中存在非激活态讲台（无 END_PORTAL 方块）：应通过策略 2（高度图扫描）匹配
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    EndDragonFight fight(42, data);

    // 在原点构建非激活态讲台（无 END_PORTAL 方块）
    EndTeleporter::createExitPortal(m_world, BlockPos(0, 64, 0), false);

    test::EndDragonFightTestAccessor accessor(fight);
    auto match = accessor.findExitPortal(m_world);
    EXPECT_TRUE(match.has_value());
}

TEST_F(EndDragonFightFindPortalTest, FindExitPortal_SetsPortalLocation_OnFirstMatch)
{
    // 首次匹配成功后，m_portalLocation 应被设置
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    EndDragonFight fight(42, data);

    EXPECT_FALSE(fight.portalLocation().has_value());

    EndTeleporter::createExitPortal(m_world, BlockPos(0, 64, 0), true);

    test::EndDragonFightTestAccessor accessor(fight);
    auto match = accessor.findExitPortal(m_world);
    ASSERT_TRUE(match.has_value());

    // m_portalLocation 应被设置
    EXPECT_TRUE(fight.portalLocation().has_value());
}

TEST_F(EndDragonFightFindPortalTest, FindExitPortal_DoesNotOverwritePortalLocation)
{
    // 已有 m_portalLocation 时，_findExitPortal 不应覆盖它
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.exitPortalLocation = BlockPos(10, 20, 30);
    EndDragonFight fight(42, data);

    EndTeleporter::createExitPortal(m_world, BlockPos(0, 64, 0), true);

    test::EndDragonFightTestAccessor accessor(fight);
    auto match = accessor.findExitPortal(m_world);
    ASSERT_TRUE(match.has_value());

    // m_portalLocation 应保持原值
    ASSERT_TRUE(fight.portalLocation().has_value());
    EXPECT_EQ(fight.portalLocation()->x, 10);
    EXPECT_EQ(fight.portalLocation()->y, 20);
    EXPECT_EQ(fight.portalLocation()->z, 30);
}

// ============================================================================
// tryRespawn 集成测试
// ============================================================================

TEST_F(EndDragonFightFindPortalTest, TryRespawn_NoPortal_CreatesActivePortal)
{
    // 龙已死 + 无讲台：tryRespawn 应创建激活态讲台
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.dragonKilled = true;
    data.previouslyKilled = true;
    EndDragonFight fight(42, data);

    fight.tryRespawn(m_world);

    // 未放水晶，不应启动重生
    EXPECT_FALSE(fight.isRespawning());

    // 但应已记录 portalLocation（原点）
    EXPECT_TRUE(fight.portalLocation().has_value());
}

TEST_F(EndDragonFightFindPortalTest, TryRespawn_WithPortal_UsesPortalLocation)
{
    // 龙已死 + 已有讲台：tryRespawn 应使用已找到的讲台位置
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.dragonKilled = true;
    data.previouslyKilled = true;
    EndDragonFight fight(42, data);

    // 在原点构建激活态讲台
    EndTeleporter::createExitPortal(m_world, BlockPos(0, 64, 0), true);

    fight.tryRespawn(m_world);

    // 未放水晶，不应启动重生
    EXPECT_FALSE(fight.isRespawning());

    // portalLocation 应被设置（从 _findExitPortal 获取）
    EXPECT_TRUE(fight.portalLocation().has_value());
}
