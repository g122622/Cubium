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

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/attribute/EnvironmentAttributeSystem.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/block/blocks/nether/NetherPortalBlock.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include <memory>
#include <gtest/gtest.h>

using namespace mc;

namespace {

/// 传送门所在位置（也是查找生成基座时的起点）
constexpr i32 PORTAL_X = 0;
constexpr i32 PORTAL_Y = 70;
constexpr i32 PORTAL_Z = 0;

/**
 * @brief 可配置"传送门刷猪灵"环境属性的测试世界
 *
 * `BaseTestWorld::environmentAttributes()` 返回空系统（布尔属性一律为默认 false），
 * 只能断言"早退"路径。这里用位置层把该属性钉成指定值，使难度概率、玩家邻近、
 * 生成基座合法性等真正的生成逻辑可以被正面断言。
 */
class PortalSpawnTestWorld final : public test::MapBackedTestWorld {
public:
    PortalSpawnTestWorld(Difficulty difficulty, bool spawnsPiglins, bool playerNearby)
        : MapBackedTestWorld(difficulty)
        , m_playerNearby(playerNearby)
    {
        const auto& attribute = world::attribute::EnvironmentAttributes::NETHER_PORTAL_SPAWNS_PIGLINS();
        m_attributes = world::attribute::EnvironmentAttributeSystemBuilder{}
                           .addPositionalLayer<bool>(attribute,
                               [spawnsPiglins](bool, const Vector3&) {
                                   return spawnsPiglins;
                               })
                           .build();
    }

    [[nodiscard]] const world::attribute::EnvironmentAttributeSystem& environmentAttributes() const override
    {
        return m_attributes;
    }

    // randomTick 在实体注册表为空时直接返回，因此必须提供一个真实实例。
    [[nodiscard]] ecs::EntityRegistry* entityRegistry() override
    {
        return &m_entityRegistry;
    }

    [[nodiscard]] bool anyPlayerCloseEnoughTo(const BlockPos&, i32) const override
    {
        return m_playerNearby;
    }

    // 只记录"是否尝试生成"，不接管实体所有权，避免测试世界持有实体生命周期。
    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override
    {
        ++m_spawnAttempts;
        return INVALID_ENTITY_ID;
    }

    [[nodiscard]] i32 spawnAttempts() const
    {
        return m_spawnAttempts;
    }

    /// 摆 1 根石质基座 + 若干门体方块，门体自 PORTAL_Y 向下堆叠。
    void buildPortalShaft(i32 portalBlocks)
    {
        setBlockState(PORTAL_X, PORTAL_Y - portalBlocks, PORTAL_Z, &VanillaBlocks::STONE->defaultState());
        for (i32 offset = 0; offset < portalBlocks; ++offset) {
            setBlockState(PORTAL_X, PORTAL_Y - offset, PORTAL_Z, &VanillaBlocks::NETHER_PORTAL->defaultState());
        }
    }

private:
    bool m_playerNearby;
    world::attribute::EnvironmentAttributeSystem m_attributes;
    ecs::EntityRegistry m_entityRegistry{"nether-portal-spawn-test"};
    i32 m_spawnAttempts = 0;
};

constexpr i32 SPAWN_ROLL_BOUND = 2000;

/**
 * @brief 找一个种子，使首次 `nextInt(2000)` 满足给定谓词
 *
 * randomTick 用 `nextInt(2000) >= difficulty` 做概率门控。为随机源实现一整套 IRandom
 * 桩并不划算，直接搜种子更简单，且结果完全确定。
 */
template <typename Predicate>
u64 _seedForRoll(Predicate predicate)
{
    for (u64 seed = 0; seed < 200000; ++seed) {
        math::Random probe(seed);
        if (predicate(probe.nextInt(SPAWN_ROLL_BOUND))) {
            return seed;
        }
    }
    return 0;
}

/**
 * @brief 驱动一次随机刻，返回本次尝试生成的次数
 *
 * @param rollPasses true 表示让概率门控命中（nextInt(2000) < difficulty），
 *                   false 表示让门控必定失败
 */
i32 _randomTick(PortalSpawnTestWorld& world, bool rollPasses)
{
    const auto before = world.spawnAttempts();
    BlockState state = VanillaBlocks::NETHER_PORTAL->defaultState();
    const i32 difficultyId = static_cast<i32>(world.difficulty());
    const u64 seed = rollPasses ? _seedForRoll([difficultyId](i32 roll) {
        return roll < difficultyId;
    })
                                : _seedForRoll([difficultyId](i32 roll) {
                                      return roll >= difficultyId;
                                  });
    math::Random random(seed);
    const BlockPos pos(PORTAL_X, PORTAL_Y, PORTAL_Z);
    VanillaBlocks::NETHER_PORTAL->randomTick(world, pos, state, random);
    return world.spawnAttempts() - before;
}

} // namespace

class NetherPortalSpawnTest : public testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // 原版实体类型注册是显式且幂等的；randomTick 取不到 ZOMBIFIED_PIGLIN 时会早退。
        entity::VanillaEntities::registerAll();
    }
};

// 和平难度即使属性为真也不生成：isSpawningMonsters() 在和平难度恒为 false。
TEST_F(NetherPortalSpawnTest, PeacefulDifficultyNeverSpawns)
{
    PortalSpawnTestWorld world(Difficulty::Peaceful, true, true);
    world.buildPortalShaft(2);
    EXPECT_EQ(_randomTick(world, true), 0);
}

// 维度未声明"传送门刷猪灵"属性时不生成（多数维度属于这种情况）。
TEST_F(NetherPortalSpawnTest, MissingDimensionAttributeNeverSpawns)
{
    PortalSpawnTestWorld world(Difficulty::Easy, false, true);
    world.buildPortalShaft(2);
    EXPECT_EQ(_randomTick(world, true), 0);
}

// 附近没有玩家时不生成（对应原版 anyPlayerCloseEnoughForSpawning）。
TEST_F(NetherPortalSpawnTest, NoNearbyPlayerNeverSpawns)
{
    PortalSpawnTestWorld world(Difficulty::Easy, true, false);
    world.buildPortalShaft(2);
    EXPECT_EQ(_randomTick(world, true), 0);
}

// 概率门控未命中时不生成。
TEST_F(NetherPortalSpawnTest, FailedDifficultyRollNeverSpawns)
{
    PortalSpawnTestWorld world(Difficulty::Easy, true, true);
    world.buildPortalShaft(2);
    EXPECT_EQ(_randomTick(world, false), 0);
}

// 生成基座不可站立（门体下方没有任何方块）时不生成。
TEST_F(NetherPortalSpawnTest, MissingSpawnBaseNeverSpawns)
{
    PortalSpawnTestWorld world(Difficulty::Easy, true, true);
    // 只放门体、不放基座：向下找第一个非门体方块会拿到 nullptr。
    world.setBlockState(PORTAL_X, PORTAL_Y, PORTAL_Z, &VanillaBlocks::NETHER_PORTAL->defaultState());
    EXPECT_EQ(_randomTick(world, true), 0);
}

// 前四项前置条件全部满足时必须真的发起一次生成。
TEST_F(NetherPortalSpawnTest, SpawnsWhenEveryPreconditionHolds)
{
    PortalSpawnTestWorld world(Difficulty::Easy, true, true);
    world.buildPortalShaft(2);
    EXPECT_EQ(_randomTick(world, true), 1);
}
