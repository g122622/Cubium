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
 * @file WitherSkullBlockTest.cpp
 * @brief 凋灵召唤（WitherSkullBlock::checkSpawn）单元测试
 *
 * 参考 MC 1.21.11 WitherSkullBlock：放置凋灵骷髅头颅构成 T 形图案
 * （3 头颅 / 3 基座 / 空气-基座-空气）时，清空图案方块并生成凋灵。
 * 覆盖：东西/南北朝向、图案不完整不生成、非凋灵头颅不触发、
 *       和平难度不生成、生成后图案方块被清除、客户端不生成。
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/boss/WitherEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/decorative/SkullBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 凋灵召唤测试用世界桩：内存方块表 + 记录生成实体
 */
class WitherSkullTestWorld final : public mc::test::BaseTestWorld {
public:
    WitherSkullTestWorld()
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return VanillaBlocks::AIR != nullptr ? &VanillaBlocks::AIR->defaultState() : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] bool isClientSide() const override
    {
        return m_isClientSide;
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return m_difficulty;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    void setClientSide(bool v)
    {
        m_isClientSide = v;
    }

    void setDifficulty(Difficulty d)
    {
        m_difficulty = d;
    }

    void setBlockAt(i32 x, i32 y, i32 z, const Block* block)
    {
        if (block != nullptr) {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(block->defaultState());
        }
    }

    [[nodiscard]] bool isAirAt(i32 x, i32 y, i32 z) const
    {
        const BlockState* s = getBlockState(x, y, z);
        return s != nullptr && s->isAir();
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const
    {
        return m_spawnedEntities;
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    bool m_isClientSide = false;
    Difficulty m_difficulty = Difficulty::Normal;
};

} // namespace

class WitherSkullBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        world_ = std::make_unique<WitherSkullTestWorld>();
    }

    /// 以 (cx, cy, cz) 为顶点头颅、沿 dx 轴（true=X / false=Z）构建完整凋灵图案
    void buildWitherPattern(i32 cx, i32 cy, i32 cz, bool alongX)
    {
        const i32 dx = alongX ? 1 : 0;
        const i32 dz = alongX ? 0 : 1;
        for (i32 i = -1; i <= 1; ++i) {
            world_->setBlockAt(cx + dx * i, cy, cz + dz * i, VanillaBlocks::WITHER_SKELETON_SKULL);
            world_->setBlockAt(cx + dx * i, cy - 1, cz + dz * i, VanillaBlocks::SOUL_SAND);
        }
        // 底层中央为基座，两侧为空气（空气为默认，无需设置）
        world_->setBlockAt(cx, cy - 2, cz, VanillaBlocks::SOUL_SAND);
    }

    std::unique_ptr<WitherSkullTestWorld> world_;
};

TEST_F(WitherSkullBlockTest, CompletePattern_AlongX_SpawnsWither)
{
    BlockPos skullPos(0, 10, 0);
    buildWitherPattern(skullPos.x, skullPos.y, skullPos.z, /*alongX=*/true);

    WitherSkullBlock::checkSpawn(*world_, skullPos);

    ASSERT_EQ(world_->spawnedEntities().size(), 1U);
    EXPECT_NE(dynamic_cast<entity::WitherEntity*>(world_->spawnedEntities()[0].get()), nullptr);
}

TEST_F(WitherSkullBlockTest, CompletePattern_AlongZ_SpawnsWither)
{
    BlockPos skullPos(0, 10, 0);
    buildWitherPattern(skullPos.x, skullPos.y, skullPos.z, /*alongX=*/false);

    WitherSkullBlock::checkSpawn(*world_, skullPos);

    ASSERT_EQ(world_->spawnedEntities().size(), 1U);
    EXPECT_NE(dynamic_cast<entity::WitherEntity*>(world_->spawnedEntities()[0].get()), nullptr);
}

TEST_F(WitherSkullBlockTest, CheckSpawn_FromPositiveSideHead_StillMatches)
{
    // 从图案右端头颅触发（pos 为右端，中心在 pos-dx）
    buildWitherPattern(0, 10, 0, /*alongX=*/true);

    WitherSkullBlock::checkSpawn(*world_, BlockPos(1, 10, 0));

    EXPECT_EQ(world_->spawnedEntities().size(), 1U);
}

TEST_F(WitherSkullBlockTest, CheckSpawn_FromNegativeSideHead_StillMatches)
{
    // 从图案左端头颅触发（pos 为左端，中心在 pos+dx）—— 曾因 center 偏移方向错误而漏命中
    buildWitherPattern(0, 10, 0, /*alongX=*/true);

    WitherSkullBlock::checkSpawn(*world_, BlockPos(-1, 10, 0));

    EXPECT_EQ(world_->spawnedEntities().size(), 1U);
}

TEST_F(WitherSkullBlockTest, CheckSpawn_FromLeftHead_AlongZ_StillMatches)
{
    // Z 轴向图案从左端头颅触发
    buildWitherPattern(0, 10, 0, /*alongX=*/false);

    WitherSkullBlock::checkSpawn(*world_, BlockPos(0, 10, -1));

    EXPECT_EQ(world_->spawnedEntities().size(), 1U);
}

TEST_F(WitherSkullBlockTest, PatternBlocksCleared_AfterSpawn)
{
    buildWitherPattern(0, 10, 0, /*alongX=*/true);

    WitherSkullBlock::checkSpawn(*world_, BlockPos(0, 10, 0));

    // 3 头颅 + 3 中层基座 + 1 底层中央基座，共 7 个方块被清除为空气
    EXPECT_TRUE(world_->isAirAt(-1, 10, 0));
    EXPECT_TRUE(world_->isAirAt(0, 10, 0));
    EXPECT_TRUE(world_->isAirAt(1, 10, 0));
    EXPECT_TRUE(world_->isAirAt(-1, 9, 0));
    EXPECT_TRUE(world_->isAirAt(0, 9, 0));
    EXPECT_TRUE(world_->isAirAt(1, 9, 0));
    EXPECT_TRUE(world_->isAirAt(0, 8, 0));
}

TEST_F(WitherSkullBlockTest, IncompletePattern_MissingHead_NoSpawn)
{
    buildWitherPattern(0, 10, 0, /*alongX=*/true);
    // 移除一个头颅（用泥土替换）
    world_->setBlockAt(1, 10, 0, VanillaBlocks::DIRT);

    WitherSkullBlock::checkSpawn(*world_, BlockPos(0, 10, 0));

    EXPECT_TRUE(world_->spawnedEntities().empty());
}

TEST_F(WitherSkullBlockTest, IncompletePattern_MissingCenterBase_NoSpawn)
{
    buildWitherPattern(0, 10, 0, /*alongX=*/true);
    // 中层中央基座被泥土替换
    world_->setBlockAt(0, 9, 0, VanillaBlocks::DIRT);

    WitherSkullBlock::checkSpawn(*world_, BlockPos(0, 10, 0));

    EXPECT_TRUE(world_->spawnedEntities().empty());
}

TEST_F(WitherSkullBlockTest, IncompletePattern_BottomCenterNotBase_NoSpawn)
{
    buildWitherPattern(0, 10, 0, /*alongX=*/true);
    // 底层中央改为空气（移除基座）
    world_->setBlockState(0, 8, 0, &VanillaBlocks::AIR->defaultState());

    WitherSkullBlock::checkSpawn(*world_, BlockPos(0, 10, 0));

    EXPECT_TRUE(world_->spawnedEntities().empty());
}

TEST_F(WitherSkullBlockTest, NonWitherSkull_NoSpawn)
{
    BlockPos skullPos(0, 10, 0);
    buildWitherPattern(skullPos.x, skullPos.y, skullPos.z, /*alongX=*/true);
    // 顶点头颅换成普通骷髅头颅（非凋灵）
    world_->setBlockAt(0, 10, 0, VanillaBlocks::SKELETON_SKULL);

    WitherSkullBlock::checkSpawn(*world_, skullPos);

    EXPECT_TRUE(world_->spawnedEntities().empty());
}

TEST_F(WitherSkullBlockTest, PeacefulDifficulty_NoSpawn)
{
    world_->setDifficulty(Difficulty::Peaceful);
    buildWitherPattern(0, 10, 0, /*alongX=*/true);

    WitherSkullBlock::checkSpawn(*world_, BlockPos(0, 10, 0));

    EXPECT_TRUE(world_->spawnedEntities().empty());
}

TEST_F(WitherSkullBlockTest, ClientSide_NoSpawn)
{
    world_->setClientSide(true);
    buildWitherPattern(0, 10, 0, /*alongX=*/true);

    WitherSkullBlock::checkSpawn(*world_, BlockPos(0, 10, 0));

    EXPECT_TRUE(world_->spawnedEntities().empty());
}

TEST_F(WitherSkullBlockTest, WitherSpawnedStartsInvulnerablePhase)
{
    buildWitherPattern(0, 10, 0, /*alongX=*/true);

    WitherSkullBlock::checkSpawn(*world_, BlockPos(0, 10, 0));

    ASSERT_EQ(world_->spawnedEntities().size(), 1U);
    auto* wither = dynamic_cast<entity::WitherEntity*>(world_->spawnedEntities()[0].get());
    ASSERT_NE(wither, nullptr);
    // 对齐 vanilla makeInvulnerable：设置 220 tick 生成无敌计时器（而非永久无敌标志）
    EXPECT_EQ(wither->getInvulTime(), 220);
    EXPECT_FALSE(wither->isInvulnerable());
}