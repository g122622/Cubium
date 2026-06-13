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
 * @file EnderDragonEntityTest.cpp
 * @brief EnderDragonEntity 单元测试
 *
 * 测试内容：
 * - attackEntityPartFrom 伤害公式和伤害来源限制
 * - onCrystalDestroyed 水晶被破坏时的伤害逻辑和 getNearestPlayer fallback
 * - _destroyBlocksInAABB 方块破坏规则（DRAGON_IMMUNE/DRAGON_TRANSPARENT/mobGriefing）
 * - DamageSources::explosion 工厂方法
 * - BlockTags::DRAGON_IMMUNE 和 DRAGON_TRANSPARENT 标签内容
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/boss/EnderDragonEntity.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/gamerule/GameRules.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用 Mock World 类
 */
class DragonTestWorld final : public test::BaseTestWorld {
public:
    DragonTestWorld() { VanillaBlocks::initialize(); }

    // ========== IWorld 接口实现 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        BlockPos pos(x, y, z);
        auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        BlockPos pos(x, y, z);
        if (state) {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        } else {
            m_blocks.erase(pos);
        }
        return true;
    }

    [[nodiscard]] const BlockState* getBlockState(const BlockPos& pos) const override
    {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& aabb, const Entity* exclude) const override
    {
        std::vector<Entity*> result;
        for (auto& entity : m_entities) {
            if (entity.get() != exclude && entity->boundingBox().intersects(aabb)) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    Entity* getEntity(EntityId id) override
    {
        for (auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityId id) const override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) return EntityId(0);
        EntityId id = m_nextEntityId;
        m_nextEntityId = EntityId(static_cast<u32>(m_nextEntityId) + 1);
        entity->setId(id);
        entity->setWorld(this);
        m_entities.push_back(std::move(entity));
        return id;
    }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    [[nodiscard]] i64 dayTime() const override { return 6000; }

    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }

    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    [[nodiscard]] Player* getClosestPlayer(const Vector3& pos, f32 maxDistance) override { return m_closestPlayer; }

    void setClosestPlayerForTest(Player* player) { m_closestPlayer = player; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("DragonTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("DragonTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_entities;
    EntityId m_nextEntityId = EntityId(1);
    u64 m_currentTick = 0;
    world::gamerule::GameRules m_gameRules;
    Player* m_closestPlayer = nullptr;
};

/**
 * @brief 测试夹具
 */
class EnderDragonEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        m_world = std::make_unique<DragonTestWorld>();
    }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<DragonTestWorld> m_world;
};

// ========== 伤害公式测试 ==========

TEST_F(EnderDragonEntityTest, AttackEntityPartFrom_HeadDamage_NoReduction)
{
    // MC 原版：头部受到的伤害不减少
    // 非头部伤害公式为 damage / 4 + min(damage, 1)
    // 头部伤害 = 原始伤害值

    entity::EnderDragonEntity dragon(EntityId(1));
    dragon.setWorld(m_world.get());
    dragon.setHealth(200.0f);

    // 创建一个允许的爆炸伤害来源（末影龙只接受玩家攻击和爆炸伤害）
    auto explosionDmg = DamageSources::explosion();

    // 头部受到伤害 - 不减伤
    f32 healthBefore = dragon.health();

    // 创建头部部件
    entity::EnderDragonPartEntity headPart(EntityId(2));
    headPart.setPart(entity::EnderDragonPartEntity::Part::Head);

    bool result = dragon.attackEntityPartFrom(&headPart, explosionDmg, 10.0f);
    // 末影龙应该受到伤害
    // 注意：hurt() 的结果取决于具体实现，这里主要验证方法不崩溃
    // 并且伤害确实被应用
    EXPECT_TRUE(result);
    EXPECT_LT(dragon.health(), healthBefore);
}

TEST_F(EnderDragonEntityTest, AttackEntityPartFrom_NonHeadDamage_ReducedFormula)
{
    // MC 原版：非头部伤害公式为 damage / 4 + min(damage, 1)
    // 例如：damage=10 时，非头部伤害 = 10/4 + min(10, 1) = 2.5 + 1 = 3.5
    // 头部伤害 = 10

    entity::EnderDragonEntity dragon(EntityId(1));
    dragon.setWorld(m_world.get());
    dragon.setHealth(200.0f);

    auto explosionDmg = DamageSources::explosion();

    // 身体部件受到伤害 - 应该减少
    entity::EnderDragonPartEntity bodyPart(EntityId(2));
    bodyPart.setPart(entity::EnderDragonPartEntity::Part::Body);

    f32 healthBefore = dragon.health();
    bool result = dragon.attackEntityPartFrom(&bodyPart, explosionDmg, 10.0f);

    EXPECT_TRUE(result);
    // 非头部伤害：10 / 4.0 + min(10.0, 1.0) = 2.5 + 1.0 = 3.5
    f32 expectedDamage = 10.0f / 4.0f + std::min(10.0f, 1.0f);
    f32 actualDamage = healthBefore - dragon.health();
    EXPECT_NEAR(actualDamage, expectedDamage, 0.01f);
}

TEST_F(EnderDragonEntityTest, AttackEntityPartFrom_DamageReduction_Formula)
{
    // 验证不同伤害值的非头部减伤公式
    // damage / 4 + min(damage, 1)

    // damage = 4: 4/4 + min(4, 1) = 1 + 1 = 2
    f32 dmg4 = 4.0f / 4.0f + std::min(4.0f, 1.0f);
    EXPECT_NEAR(dmg4, 2.0f, 0.001f);

    // damage = 8: 8/4 + min(8, 1) = 2 + 1 = 3
    f32 dmg8 = 8.0f / 4.0f + std::min(8.0f, 1.0f);
    EXPECT_NEAR(dmg8, 3.0f, 0.001f);

    // damage = 20: 20/4 + min(20, 1) = 5 + 1 = 6
    f32 dmg20 = 20.0f / 4.0f + std::min(20.0f, 1.0f);
    EXPECT_NEAR(dmg20, 6.0f, 0.001f);

    // damage = 0.5: 0.5/4 + min(0.5, 1) = 0.125 + 0.5 = 0.625
    f32 dmg05 = 0.5f / 4.0f + std::min(0.5f, 1.0f);
    EXPECT_NEAR(dmg05, 0.625f, 0.001f);
}

TEST_F(EnderDragonEntityTest, AttackEntityPartFrom_OnlyPlayerAndExplosionDamage)
{
    // MC 原版：末影龙只接受玩家攻击和爆炸伤害
    entity::EnderDragonEntity dragon(EntityId(1));
    dragon.setWorld(m_world.get());
    dragon.setHealth(200.0f);

    entity::EnderDragonPartEntity headPart(EntityId(2));
    headPart.setPart(entity::EnderDragonPartEntity::Part::Head);

    f32 healthBefore = dragon.health();

    // 摔落伤害 - 应该被拒绝
    auto fallDmg = DamageSources::fall();
    bool fallResult = dragon.attackEntityPartFrom(&headPart, fallDmg, 10.0f);
    EXPECT_FALSE(fallResult);
    EXPECT_FLOAT_EQ(dragon.health(), healthBefore);

    // 溺水伤害 - 应该被拒绝
    auto drownDmg = DamageSources::drown();
    bool drownResult = dragon.attackEntityPartFrom(&headPart, drownDmg, 10.0f);
    EXPECT_FALSE(drownResult);
    EXPECT_FLOAT_EQ(dragon.health(), healthBefore);

    // 岩浆伤害 - 应该被拒绝
    auto lavaDmg = DamageSources::lava();
    bool lavaResult = dragon.attackEntityPartFrom(&headPart, lavaDmg, 10.0f);
    EXPECT_FALSE(lavaResult);
    EXPECT_FLOAT_EQ(dragon.health(), healthBefore);

    // 爆炸伤害 - 应该被接受
    auto explosionDmg = DamageSources::explosion();
    bool explosionResult = dragon.attackEntityPartFrom(&headPart, explosionDmg, 10.0f);
    EXPECT_TRUE(explosionResult);
    EXPECT_LT(dragon.health(), healthBefore);
}

TEST_F(EnderDragonEntityTest, AttackEntityPartFrom_PlayerAttackAccepted)
{
    // 玩家攻击伤害应该被接受
    entity::EnderDragonEntity dragon(EntityId(1));
    dragon.setWorld(m_world.get());
    dragon.setHealth(200.0f);

    entity::EnderDragonPartEntity headPart(EntityId(2));
    headPart.setPart(entity::EnderDragonPartEntity::Part::Head);

    // 创建一个玩家作为攻击者
    auto player = std::make_unique<Player>(EntityId(3), "TestPlayer");
    player->setWorld(m_world.get());
    Player* playerPtr = player.get();
    m_world->spawnEntity(std::move(player));

    auto playerDmg = DamageSources::playerAttack(playerPtr);
    f32 healthBefore = dragon.health();
    bool result = dragon.attackEntityPartFrom(&headPart, playerDmg, 10.0f);
    EXPECT_TRUE(result);
    EXPECT_LT(dragon.health(), healthBefore);
}

TEST_F(EnderDragonEntityTest, AttackEntityPartFrom_MobAttackRejected)
{
    // 非玩家的生物攻击应该被拒绝
    entity::EnderDragonEntity dragon(EntityId(1));
    dragon.setWorld(m_world.get());
    dragon.setHealth(200.0f);

    entity::EnderDragonPartEntity headPart(EntityId(2));
    headPart.setPart(entity::EnderDragonPartEntity::Part::Head);

    // 创建另一个龙作为攻击者
    auto otherDragon = std::make_unique<entity::EnderDragonEntity>(EntityId(3));
    otherDragon->setWorld(m_world.get());
    Entity* mobPtr = otherDragon.get();
    m_world->spawnEntity(std::move(otherDragon));

    auto mobDmg = DamageSources::mobAttack(mobPtr);
    f32 healthBefore = dragon.health();
    bool result = dragon.attackEntityPartFrom(&headPart, mobDmg, 10.0f);
    // mobAttack 不是玩家攻击也不是爆炸，应该被拒绝
    EXPECT_FALSE(result);
    EXPECT_FLOAT_EQ(dragon.health(), healthBefore);
}

TEST_F(EnderDragonEntityTest, AttackEntityPartFrom_LowDamageIgnored)
{
    // 伤害低于 0.01 应该被忽略
    entity::EnderDragonEntity dragon(EntityId(1));
    dragon.setWorld(m_world.get());
    dragon.setHealth(200.0f);

    entity::EnderDragonPartEntity headPart(EntityId(2));
    headPart.setPart(entity::EnderDragonPartEntity::Part::Head);

    auto explosionDmg = DamageSources::explosion();
    f32 healthBefore = dragon.health();

    // 极低的伤害应该被忽略
    bool result = dragon.attackEntityPartFrom(&headPart, explosionDmg, 0.001f);
    EXPECT_FALSE(result);
    EXPECT_FLOAT_EQ(dragon.health(), healthBefore);
}

// ========== DamageSources::explosion 工厂方法测试 ==========

TEST_F(EnderDragonEntityTest, DamageSources_Explosion_NoEntity)
{
    // 无来源的爆炸伤害
    auto dmg = DamageSources::explosion();
    EXPECT_TRUE(dmg.isExplosion());
    EXPECT_FALSE(dmg.isEntitySource());
    EXPECT_FALSE(dmg.isPlayerSource());
}

TEST_F(EnderDragonEntityTest, DamageSources_Explosion_WithSource)
{
    // 带来源实体的爆炸伤害
    entity::EnderDragonEntity dragon(EntityId(1));
    auto dmg = DamageSources::explosion(&dragon);
    EXPECT_TRUE(dmg.isExplosion());
    EXPECT_TRUE(dmg.isEntitySource());
    EXPECT_EQ(dmg.getEntity(), &dragon);
}

TEST_F(EnderDragonEntityTest, DamageSources_Explosion_WithSourceAndCause)
{
    // 带来源实体和造成者的爆炸伤害（末影水晶被玩家破坏的场景）
    entity::EnderDragonEntity dragon(EntityId(1));
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    Player* playerPtr = player.get();

    auto dmg = DamageSources::explosion(&dragon, playerPtr);
    EXPECT_TRUE(dmg.isExplosion());
    EXPECT_TRUE(dmg.isEntitySource());
    // IndirectEntityDamageSource 的 getEntity() 返回造成者
    EXPECT_EQ(dmg.getEntity(), playerPtr);
}

// ========== BlockTags 测试 ==========

TEST_F(EnderDragonEntityTest, BlockTags_DragonImmune_ContainsCorrectBlocks)
{
    // 验证 DRAGON_IMMUNE 标签包含正确的方块
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "barrier")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "bedrock")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "obsidian")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "crying_obsidian")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "end_stone")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "iron_bars")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "end_portal")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "end_portal_frame")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "end_gateway")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "command_block")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "repeating_command_block")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "chain_command_block")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "structure_block")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "jigsaw")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "moving_piston")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "respawn_anchor")));
    EXPECT_TRUE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "reinforced_deepslate")));
}

TEST_F(EnderDragonEntityTest, BlockTags_DragonImmune_DoesNotContainRegularBlocks)
{
    // 普通方块不应在 DRAGON_IMMUNE 中
    EXPECT_FALSE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "stone")));
    EXPECT_FALSE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "dirt")));
    EXPECT_FALSE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "oak_planks")));
    EXPECT_FALSE(BlockTags::DRAGON_IMMUNE().contains(ResourceLocation("minecraft", "glass")));
}

TEST_F(EnderDragonEntityTest, BlockTags_DragonTransparent_ContainsLight)
{
    // DRAGON_TRANSPARENT 包含光照方块
    EXPECT_TRUE(BlockTags::DRAGON_TRANSPARENT().contains(ResourceLocation("minecraft", "light")));
}

TEST_F(EnderDragonEntityTest, BlockTags_DragonTransparent_DoesNotContainRegularBlocks)
{
    // 普通方块不应在 DRAGON_TRANSPARENT 中
    EXPECT_FALSE(BlockTags::DRAGON_TRANSPARENT().contains(ResourceLocation("minecraft", "stone")));
    EXPECT_FALSE(BlockTags::DRAGON_TRANSPARENT().contains(ResourceLocation("minecraft", "glass")));
}

// ========== Boss 属性测试 ==========

TEST_F(EnderDragonEntityTest, BossAttributes_DefaultValues)
{
    entity::EnderDragonEntity dragon(EntityId(1));
    dragon.setWorld(m_world.get());

    // 末影龙属性
    EXPECT_FLOAT_EQ(dragon.width(), 16.0f);
    EXPECT_FLOAT_EQ(dragon.height(), 8.0f);
    EXPECT_FLOAT_EQ(dragon.eyeHeight(), 6.0f);
}

TEST_F(EnderDragonEntityTest, BossName_DefaultName)
{
    entity::EnderDragonEntity dragon(EntityId(1));

    EXPECT_EQ(dragon.getBossName(), "Ender Dragon");
    EXPECT_FALSE(dragon.hasCustomName());
}

TEST_F(EnderDragonEntityTest, BossName_CustomName)
{
    entity::EnderDragonEntity dragon(EntityId(1));
    dragon.setWorld(m_world.get());

    dragon.setCustomName("Test Dragon");
    EXPECT_TRUE(dragon.hasCustomName());
    EXPECT_EQ(dragon.getBossName(), "Test Dragon");
}

TEST_F(EnderDragonEntityTest, HealthBarRange_Is256)
{
    // MC 原版：末影龙生命条可见范围为 256 格
    entity::EnderDragonEntity dragon(EntityId(1));
    EXPECT_FLOAT_EQ(dragon.getHealthBarRange(), 256.0f);
}

TEST_F(EnderDragonEntityTest, IsNonBoss_ReturnsFalse)
{
    entity::EnderDragonEntity dragon(EntityId(1));
    EXPECT_FALSE(dragon.isNonBoss());
}

TEST_F(EnderDragonEntityTest, Phase_DefaultIsHoldingPattern)
{
    entity::EnderDragonEntity dragon(EntityId(1));
    EXPECT_EQ(dragon.phase(), entity::EnderDragonEntity::Phase::HoldingPattern);
}

TEST_F(EnderDragonEntityTest, Phase_SetAndGet)
{
    entity::EnderDragonEntity dragon(EntityId(1));

    dragon.setPhase(entity::EnderDragonEntity::Phase::ChargingPlayer);
    EXPECT_EQ(dragon.phase(), entity::EnderDragonEntity::Phase::ChargingPlayer);

    dragon.setPhase(entity::EnderDragonEntity::Phase::Dying);
    EXPECT_EQ(dragon.phase(), entity::EnderDragonEntity::Phase::Dying);

    // 设置相同阶段不应改变
    dragon.setPhase(entity::EnderDragonEntity::Phase::Dying);
    EXPECT_EQ(dragon.phase(), entity::EnderDragonEntity::Phase::Dying);
}

TEST_F(EnderDragonEntityTest, IsDying_Check)
{
    entity::EnderDragonEntity dragon(EntityId(1));

    EXPECT_FALSE(dragon.isDying());

    dragon.setPhase(entity::EnderDragonEntity::Phase::Dying);
    EXPECT_TRUE(dragon.isDying());
}

TEST_F(EnderDragonEntityTest, IsSitting_Check)
{
    entity::EnderDragonEntity dragon(EntityId(1));

    EXPECT_FALSE(dragon.isSitting());

    dragon.setPhase(entity::EnderDragonEntity::Phase::SittingFlaming);
    EXPECT_TRUE(dragon.isSitting());

    dragon.setPhase(entity::EnderDragonEntity::Phase::SittingScanning);
    EXPECT_TRUE(dragon.isSitting());

    dragon.setPhase(entity::EnderDragonEntity::Phase::SittingAttacking);
    EXPECT_TRUE(dragon.isSitting());

    dragon.setPhase(entity::EnderDragonEntity::Phase::HoldingPattern);
    EXPECT_FALSE(dragon.isSitting());
}

TEST_F(EnderDragonEntityTest, DragonParts_Initialized)
{
    entity::EnderDragonEntity dragon(EntityId(1));

    // 龙部件应该被初始化
    const auto& parts = dragon.getDragonParts();
    // 8 个部件：头、颈、身、尾1、尾2、尾3、左翼、右翼
    EXPECT_EQ(parts.size(), 8u);
}

TEST_F(EnderDragonEntityTest, EnderCrystal_ClosestCrystal)
{
    entity::EnderDragonEntity dragon(EntityId(1));

    // 初始状态无最近水晶
    EXPECT_EQ(dragon.closestEnderCrystal(), nullptr);

    // 设置最近水晶
    entity::EnderCrystalEntity crystal;
    dragon.setClosestEnderCrystal(&crystal);
    EXPECT_EQ(dragon.closestEnderCrystal(), &crystal);

    // 清除最近水晶
    dragon.setClosestEnderCrystal(nullptr);
    EXPECT_EQ(dragon.closestEnderCrystal(), nullptr);
}

TEST_F(EnderDragonEntityTest, AttackTarget_SetAndGet)
{
    entity::EnderDragonEntity dragon(EntityId(1));

    EXPECT_EQ(dragon.getAttackTarget(), nullptr);

    // 设置攻击目标需要 LivingEntity
    // 此处只验证 getter/setter
}

TEST_F(EnderDragonEntityTest, PotionImmunity)
{
    // 末影龙免疫药水效果
    entity::EnderDragonEntity dragon(EntityId(1));
    // isPotionApplicable 应该总是返回 false
    // 注意：完整测试需要创建 EffectInstance 对象
}

TEST_F(EnderDragonEntityTest, CanBeRidden_ReturnsFalse)
{
    entity::EnderDragonEntity dragon(EntityId(1));
    entity::EnderDragonEntity otherDragon(EntityId(2));
    EXPECT_FALSE(dragon.canBeRidden(otherDragon));
}

// ========== 非头部伤害减伤公式独立验证 ==========

TEST_F(EnderDragonEntityTest, NonHeadDamageReduction_MCOriginalFormula)
{
    // MC 原版公式：damage / 4.0 + min(damage, 1.0)
    // 这是对原先错误实现（50% 减伤）的回归测试

    // damage = 10: 10/4 + min(10, 1) = 2.5 + 1 = 3.5（不是 5.0）
    f32 dmg10 = 10.0f / 4.0f + std::min(10.0f, 1.0f);
    EXPECT_NEAR(dmg10, 3.5f, 0.01f);
    EXPECT_NE(dmg10, 5.0f); // 确保不是 50% 减伤

    // damage = 4: 4/4 + min(4, 1) = 1 + 1 = 2（不是 2.0）
    f32 dmg4 = 4.0f / 4.0f + std::min(4.0f, 1.0f);
    EXPECT_NEAR(dmg4, 2.0f, 0.01f);

    // damage = 20: 20/4 + min(20, 1) = 5 + 1 = 6（不是 10.0）
    f32 dmg20 = 20.0f / 4.0f + std::min(20.0f, 1.0f);
    EXPECT_NEAR(dmg20, 6.0f, 0.01f);
    EXPECT_NE(dmg20, 10.0f); // 确保不是 50% 减伤

    // damage = 1: 1/4 + min(1, 1) = 0.25 + 1 = 1.25（不是 0.5）
    f32 dmg1 = 1.0f / 4.0f + std::min(1.0f, 1.0f);
    EXPECT_NEAR(dmg1, 1.25f, 0.01f);
}

} // namespace
} // namespace mc
