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
 * @file CreateWitherRoseTest.cpp
 * @brief LivingEntity::createWitherRose 行为测试
 *
 * 参考 MC 1.21.11 LivingEntity.createWitherRose（LivingEntity.java:1463-1482）：
 * 仅当击杀者为凋灵（WitherBoss）时触发：
 *   - MOB_GRIEFING 开启，且死亡位置为空气、凋零玫瑰可在该处存活 → 放置凋零玫瑰方块；
 *   - 否则 → 掉落一个凋零玫瑰物品实体。
 * 击杀者非凋灵时不触发任何行为。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"

using namespace mc;

namespace {

/**
 * @brief 记录 setBlockState 与 spawnEntity 的测试世界桩
 *
 * createWitherRose 仅依赖 getBlockState（判定空气/下方支撑）、setBlockState（放置方块）
 * 与 spawnEntity（掉落物品实体），这里对三者做最小实现并记录调用结果。
 */
class WitherRoseTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        // 未显式设置的方块视为空气（死亡位置默认空气）
        return VanillaBlocks::AIR != nullptr ? &VanillaBlocks::AIR->defaultState() : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state;
        ++m_setBlockCalls;
        return true;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    /// 预设某坐标的方块状态（不增加 setBlockState 计数）
    void presetBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[BlockPos(x, y, z)] = state; }

    [[nodiscard]] const BlockState* blockAt(i32 x, i32 y, i32 z) const
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : nullptr;
    }

    [[nodiscard]] i32 setBlockCallCount() const { return m_setBlockCalls; }
    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    i32 m_setBlockCalls = 0;
};

/**
 * @brief 测试用活体实体（LivingEntity 直接子类）
 *
 * LivingEntity 为抽象类，须派生具体类方可实例化。本类仅暴露 createWitherRose
 * 供测试直接调用。
 */
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
        : LivingEntity(id, nullptr, registry)
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

} // namespace

class CreateWitherRoseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        // createWitherRose 掉落路径构造 ItemStack(Items::WITHER_ROSE)，需先注册物品
        Items::initialize();
    }
};

// ==================== 击杀者为凋灵 ====================

TEST_F(CreateWitherRoseTest, WitherKill_GriefingOn_AirAndSoulSandBelow_PlacesBlock)
{
    // 前置：死亡位置为空气，下方为灵魂沙（凋零玫瑰可存活）
    WitherRoseTestWorld world;
    world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);
    // 实体位于 (0, 0, 0)，下方 (0,-1,0) 为灵魂沙
    world.presetBlock(0, -1, 0, &VanillaBlocks::SOUL_SAND->defaultState());

    TestLivingEntity victim(EntityInstanceId(1), mc::test::testEcsRegistry());
    victim.setWorld(&world);
    victim.setPosition(0.0f, 0.0f, 0.0f);

    TestLivingEntity wither(EntityInstanceId(2), mc::test::testEcsRegistry());
    wither.setTypeId("minecraft:wither");

    const i32 before = world.setBlockCallCount();
    victim.createWitherRose(&wither);

    // 应在死亡位置放置凋零玫瑰方块，且不掉落物品
    ASSERT_EQ(world.setBlockCallCount(), before + 1);
    const BlockState* placed = world.blockAt(0, 0, 0);
    ASSERT_NE(placed, nullptr);
    EXPECT_TRUE(placed->is(VanillaBlocks::WITHER_ROSE));
    EXPECT_TRUE(world.spawnedEntities().empty());
}

TEST_F(CreateWitherRoseTest, WitherKill_GriefingOff_DropsItem)
{
    // MOB_GRIEFING 关闭 → 不放置方块，改为掉落凋零玫瑰物品
    WitherRoseTestWorld world;
    world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);
    world.presetBlock(0, -1, 0, &VanillaBlocks::SOUL_SAND->defaultState());

    TestLivingEntity victim(EntityInstanceId(1), mc::test::testEcsRegistry());
    victim.setWorld(&world);
    victim.setPosition(0.0f, 0.0f, 0.0f);

    TestLivingEntity wither(EntityInstanceId(2), mc::test::testEcsRegistry());
    wither.setTypeId("minecraft:wither");

    victim.createWitherRose(&wither);

    EXPECT_EQ(world.setBlockCallCount(), 0);
    ASSERT_EQ(world.spawnedEntities().size(), 1U);
    auto* itemEntity = dynamic_cast<ItemEntity*>(world.spawnedEntities()[0].get());
    ASSERT_NE(itemEntity, nullptr);
    EXPECT_EQ(itemEntity->getItemStack().getItem(), Items::WITHER_ROSE);
}

TEST_F(CreateWitherRoseTest, WitherKill_GriefingOn_UnsuitableGround_DropsItem)
{
    // MOB_GRIEFING 开启但下方为石头（凋零玫瑰不可存活）→ 掉落物品
    WitherRoseTestWorld world;
    world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);
    world.presetBlock(0, -1, 0, &VanillaBlocks::STONE->defaultState());

    TestLivingEntity victim(EntityInstanceId(1), mc::test::testEcsRegistry());
    victim.setWorld(&world);
    victim.setPosition(0.0f, 0.0f, 0.0f);

    TestLivingEntity wither(EntityInstanceId(2), mc::test::testEcsRegistry());
    wither.setTypeId("minecraft:wither");

    victim.createWitherRose(&wither);

    EXPECT_EQ(world.setBlockCallCount(), 0);
    ASSERT_EQ(world.spawnedEntities().size(), 1U);
    EXPECT_NE(dynamic_cast<ItemEntity*>(world.spawnedEntities()[0].get()), nullptr);
}

TEST_F(CreateWitherRoseTest, WitherKill_GriefingOn_NetherrackBelow_PlacesBlock)
{
    // 下界岩也属于凋零玫瑰的可存活面
    WitherRoseTestWorld world;
    world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);
    world.presetBlock(0, -1, 0, &VanillaBlocks::NETHERRACK->defaultState());

    TestLivingEntity victim(EntityInstanceId(1), mc::test::testEcsRegistry());
    victim.setWorld(&world);
    victim.setPosition(0.0f, 0.0f, 0.0f);

    TestLivingEntity wither(EntityInstanceId(2), mc::test::testEcsRegistry());
    wither.setTypeId("minecraft:wither");

    victim.createWitherRose(&wither);

    const BlockState* placed = world.blockAt(0, 0, 0);
    ASSERT_NE(placed, nullptr);
    EXPECT_TRUE(placed->is(VanillaBlocks::WITHER_ROSE));
    EXPECT_TRUE(world.spawnedEntities().empty());
}

// ==================== 击杀者非凋灵 ====================

TEST_F(CreateWitherRoseTest, NonWitherKill_DoesNothing)
{
    // 非凋灵击杀者（骷髅）→ 既不放置方块也不掉落物品
    WitherRoseTestWorld world;
    world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);
    world.presetBlock(0, -1, 0, &VanillaBlocks::SOUL_SAND->defaultState());

    TestLivingEntity victim(EntityInstanceId(1), mc::test::testEcsRegistry());
    victim.setWorld(&world);
    victim.setPosition(0.0f, 0.0f, 0.0f);

    TestLivingEntity skeleton(EntityInstanceId(2), mc::test::testEcsRegistry());
    skeleton.setTypeId("minecraft:skeleton");

    victim.createWitherRose(&skeleton);

    EXPECT_EQ(world.setBlockCallCount(), 0);
    EXPECT_TRUE(world.spawnedEntities().empty());
}

TEST_F(CreateWitherRoseTest, NullKillCredit_DoesNothing)
{
    // 无击杀贡献者（环境伤害致死）→ 不触发
    WitherRoseTestWorld world;
    world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);

    TestLivingEntity victim(EntityInstanceId(1), mc::test::testEcsRegistry());
    victim.setWorld(&world);
    victim.setPosition(0.0f, 0.0f, 0.0f);

    victim.createWitherRose(nullptr);

    EXPECT_EQ(world.setBlockCallCount(), 0);
    EXPECT_TRUE(world.spawnedEntities().empty());
}