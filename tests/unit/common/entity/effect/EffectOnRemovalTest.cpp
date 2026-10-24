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

// 效果"附体实体移除/受击"回调测试。
//
// 覆盖 LivingEntity::onRemoval 的 triggerOnDeathMobEffects 链路（对齐 vanilla
// LivingEntity.remove:701-712 + MobEffectInstance.onMobRemoved）与 INFESTED 的受击回调：
//   - WEAVING + KILLED + mobGriefing=true → 实体周围生成蜘蛛网
//   - WEAVING + KILLED + mobGriefing=false 且非玩家 → 不生成
//   - WEAVING + DISCARDED → 不生成（回调仅对 KILLED 生效）
//   - 移除后效果列表清空；卸载类原因（UnloadedToChunk）不清空
//   - OOZING + KILLED → 生成小史莱姆
//   - WindChargeExplosionContext：屏障抗性无穷大、不破坏方块、不伤害实体

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/RemovalReason.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectOnRemoval.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/BaseBlocks.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionContext.hpp"
#include "common/world/gamerule/GameRules.hpp"

using namespace mc;

namespace {

/// 普通测试生物：用于承载效果并观察移除回调
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

/// 测试世界：记录 setBlockState 调用并保存当前方块状态，追踪 spawnEntity 生成的实体
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        m_blocks[pos] = state;
        m_setBlocks.push_back(pos);
        return true;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (entity == nullptr) {
            return EntityInstanceId(0);
        }
        m_spawned.push_back(entity->getTypeId());
        return EntityInstanceId(static_cast<u32>(m_spawned.size()) + 100);
    }

    /// 预置某位置为实心完整方块（用于满足蛛网的支撑判定）
    void putSolid(i32 x, i32 y, i32 z, const BlockState* state)
    {
        m_blocks[BlockPos(x, y, z)] = state;
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    std::vector<BlockPos> m_setBlocks;
    std::vector<std::string> m_spawned;
};

} // namespace

class EffectOnRemovalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::ensureInitialized();
        VanillaBlocks::initialize();
        BlockTags::initialize();
        mc::entity::VanillaEntities::registerAll();
    }

    TestWorld m_world;
};

// WEAVING + mobGriefing=true：实体周围（3×3×3 立方体）每格均可替换且下方为实心方块时，
// 击杀移除会生成 2~3 个蜘蛛网。
TEST_F(EffectOnRemovalTest, WeavingKilledSpawnsCobwebs)
{
    TestLivingEntity entity;
    entity.setWorld(&m_world);
    entity.setPosition(0.5f, 64.0f, 0.5f);
    entity.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Weaving, 600));

    // 整个 3×3×3 邻域：候选格为空气（可替换），支撑格为石头
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* air = &block_registry::BaseBlocks::AIR->defaultState();
    for (i32 x = -1; x <= 1; ++x) {
        for (i32 z = -1; z <= 1; ++z) {
            m_world.putSolid(x, 62, z, stone);
            m_world.putSolid(x, 63, z, stone);
            m_world.putSolid(x, 64, z, air);
            m_world.putSolid(x, 65, z, air);
        }
    }
    m_world.putSolid(0, 63, 0, stone); // 实体脚下

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true);

    entity.onRemoval(RemovalReason::Killed);

    EXPECT_GE(m_world.m_setBlocks.size(), 2u);
    EXPECT_LE(m_world.m_setBlocks.size(), 3u);
    for (const auto& pos : m_world.m_setBlocks) {
        const BlockState* placed = m_world.getBlockState(pos.x, pos.y, pos.z);
        ASSERT_NE(placed, nullptr);
        EXPECT_TRUE(placed->is(VanillaBlocks::COBWEB));
        // 生成的蜘蛛网都在实体周围 3×3 水平范围内
        EXPECT_LE(std::abs(pos.x), 1);
        EXPECT_LE(std::abs(pos.z), 1);
    }
    // 回调后效果列表被清空
    EXPECT_TRUE(entity.effectManager().getAllEffects().empty());
}

// WEAVING + mobGriefing=false 且实体非玩家：不生成蜘蛛网（原版仅玩家绕过该规则）。
TEST_F(EffectOnRemovalTest, WeavingHonoursMobGriefingForMobs)
{
    TestLivingEntity entity;
    entity.setWorld(&m_world);
    entity.setPosition(0.5f, 64.0f, 0.5f);
    entity.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Weaving, 600));

    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    m_world.putSolid(0, 63, 0, stone);
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false);

    entity.onRemoval(RemovalReason::Killed);

    EXPECT_TRUE(m_world.m_setBlocks.empty());
    EXPECT_TRUE(entity.effectManager().getAllEffects().empty());
}

// 非 KILLED 移除（DISCARDED）：不触发效果移除回调，但效果仍被清空。
TEST_F(EffectOnRemovalTest, DiscardedRemovalSkipsEffectCallbacks)
{
    TestLivingEntity entity;
    entity.setWorld(&m_world);
    entity.setPosition(0.5f, 64.0f, 0.5f);
    entity.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Weaving, 600));
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true);

    entity.onRemoval(RemovalReason::Discarded);

    EXPECT_TRUE(m_world.m_setBlocks.empty());
    EXPECT_TRUE(entity.effectManager().getAllEffects().empty());
}

// 卸载类移除原因（UnloadedToChunk）不清空效果，避免丢失存档中的效果状态。
TEST_F(EffectOnRemovalTest, UnloadRemovalKeepsEffects)
{
    TestLivingEntity entity;
    entity.setWorld(&m_world);
    entity.setPosition(0.5f, 64.0f, 0.5f);
    entity.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Speed, 600));

    entity.onRemoval(RemovalReason::UnloadedToChunk);

    EXPECT_EQ(entity.effectManager().getAllEffects().size(), 1u);
    EXPECT_TRUE(entity.hasEffect(entity::effect::EffectType::Speed));
}

// OOZING + KILLED：生成 2 只史莱姆（maxEntityCramming 默认 24，附近无史莱姆）。
TEST_F(EffectOnRemovalTest, OozingKilledSpawnsSlimes)
{
    TestLivingEntity entity;
    entity.setWorld(&m_world);
    entity.setPosition(0.5f, 64.0f, 0.5f);
    entity.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Oozing, 600));

    entity.onRemoval(RemovalReason::Killed);

    ASSERT_EQ(m_world.m_spawned.size(), 2u);
    for (const auto& typeId : m_world.m_spawned) {
        EXPECT_EQ(typeId, mc::entity::VanillaEntityTypeKeys::SLIME->name());
    }
}

// WindChargeExplosionContext：屏障/基岩视为无穷大抗性，其余方块走默认抗性。
TEST_F(EffectOnRemovalTest, WindChargeContextTreatsBarrierAsImmune)
{
    world::explosion::WindChargeExplosionContext context(nullptr);

    // BLOCKS_WIND_CHARGE_EXPLOSIONS 标签成员：barrier / bedrock
    EXPECT_TRUE(BlockTags::BLOCKS_WIND_CHARGE_EXPLOSIONS().contains(ResourceLocation("minecraft:barrier")));
    EXPECT_TRUE(BlockTags::BLOCKS_WIND_CHARGE_EXPLOSIONS().contains(ResourceLocation("minecraft:bedrock")));
    EXPECT_FALSE(BlockTags::BLOCKS_WIND_CHARGE_EXPLOSIONS().contains(ResourceLocation("minecraft:stone")));

    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(context.canDestroyBlock(stone, 1.0f));
}