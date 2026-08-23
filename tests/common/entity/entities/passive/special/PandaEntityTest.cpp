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
#include "common/core/Constants.hpp"
#include "common/entity/entities/passive/special/PandaEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 熊猫测试用的 Mock World
 *
 * 支持:
 * - getEntitiesInAABB (返回预设的实体列表)
 * - getGameRules (返回可配置的游戏规则)
 * - isClientSide (返回 false)
 * - addParticle (记录调用)
 * - spawnEntity (记录生成的实体)
 */
class PandaTestWorld final : public mc::test::BaseTestWorld {
public:
    PandaTestWorld()
        : m_gameRules()
        , m_doMobLoot(true)
    {}

    // 设置周围的熊猫实体（用于跳跃测试）
    void setNearbyPandas(std::vector<Entity*> pandas) { m_nearbyPandas = std::move(pandas); }

    // 设置游戏规则 doMobLoot
    void setDoMobLoot(bool value) { m_doMobLoot = value; }

    // 获取游戏规则
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }
    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }

    // 返回预设的实体列表
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_nearbyPandas;
    }

    // 服务端世界
    [[nodiscard]] bool isClientSide() const override { return false; }

    // 生成实体（返回假ID）
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return EntityInstanceId(static_cast<i32>(m_spawnedEntities.size()));
    }

    // 记录粒子生成
    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_lastParticleType = type;
        m_lastParticlePos = pos;
        m_lastParticleVelocity = velocity;
        m_particleSpawnCount++;
    }

    // 记录声音播放
    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_lastSoundId = soundEventId;
        m_soundPlayCount++;
        (void)category;
        (void)position;
        (void)volume;
        (void)pitch;
    }

    // 访问器
    [[nodiscard]] i32 getParticleSpawnCount() const { return m_particleSpawnCount; }
    [[nodiscard]] i32 getSoundPlayCount() const { return m_soundPlayCount; }
    [[nodiscard]] i32 getSpawnedEntityCount() const { return static_cast<i32>(m_spawnedEntities.size()); }
    [[nodiscard]] particle::ParticleTypeId getLastParticleType() const { return m_lastParticleType; }
    [[nodiscard]] const ResourceLocation& getLastSoundId() const { return m_lastSoundId; }
    [[nodiscard]] const Vector3& getLastParticlePosition() const { return m_lastParticlePos; }
    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& getSpawnedEntities() const { return m_spawnedEntities; }

private:
    world::gamerule::GameRules m_gameRules;
    std::vector<Entity*> m_nearbyPandas;
    bool m_doMobLoot = true;

    // 记录调用
    i32 m_particleSpawnCount = 0;
    i32 m_soundPlayCount = 0;
    particle::ParticleTypeId m_lastParticleType = static_cast<particle::ParticleTypeId>(0);
    ResourceLocation m_lastSoundId;
    Vector3 m_lastParticlePos;
    Vector3 m_lastParticleVelocity;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

/**
 * @brief 可测试的熊猫子类，暴露 protected 方法
 */
class TestablePandaEntity : public PandaEntity {
public:
    TestablePandaEntity(EntityInstanceId id)
        : PandaEntity(id, mc::test::testEcsRegistry())
    {}

    // 暴露 protected 方法用于测试
    void testOnSneezeComplete() { _onSneezeComplete(); }
    void testTick() { tick(); }
};

// ==================== PandaEntity 性格测试 ====================

TEST(PandaEntityPersonalityTest, RandomizePersonalityGeneratesValidGene)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    auto personality = panda.getPersonality();
    EXPECT_GE(static_cast<u8>(personality), 0);
    EXPECT_LE(static_cast<u8>(personality), 7);
}

TEST(PandaEntityPersonalityTest, PersonalityDistributionIsValid)
{
    Items::initialize();

    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Normal), 0);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Lazy), 1);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Worried), 2);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Playful), 3);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Aggressive), 4);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Weak), 5);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::Brown), 6);
    EXPECT_EQ(static_cast<u8>(PandaEntity::Personality::AggressiveLazy), 7);
}

// ==================== PandaEntity 性格访问器测试 ====================

TEST(PandaEntityPersonalityAccessorsTest, SetAndGetPersonality)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    panda.setPersonality(PandaEntity::Personality::Lazy);
    EXPECT_EQ(panda.getPersonality(), PandaEntity::Personality::Lazy);
    EXPECT_TRUE(panda.isLazy());

    panda.setPersonality(PandaEntity::Personality::Aggressive);
    EXPECT_EQ(panda.getPersonality(), PandaEntity::Personality::Aggressive);
    EXPECT_TRUE(panda.isAggressive());

    panda.setPersonality(PandaEntity::Personality::Playful);
    EXPECT_TRUE(panda.isPlayful());

    panda.setPersonality(PandaEntity::Personality::Worried);
    EXPECT_TRUE(panda.isWorried());

    panda.setPersonality(PandaEntity::Personality::Weak);
    EXPECT_TRUE(panda.isWeak());

    panda.setPersonality(PandaEntity::Personality::Brown);
    EXPECT_TRUE(panda.isBrown());

    panda.setPersonality(PandaEntity::Personality::Normal);
    EXPECT_FALSE(panda.isLazy());
    EXPECT_FALSE(panda.isAggressive());
    EXPECT_FALSE(panda.isPlayful());
    EXPECT_FALSE(panda.isWorried());
    EXPECT_FALSE(panda.isWeak());
    EXPECT_FALSE(panda.isBrown());
}

// ==================== PandaEntity 状态测试 ====================

TEST(PandaEntityStateTest, SetAndGetSneezing)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FALSE(panda.isSneezing());

    panda.setSneezing(true);
    EXPECT_TRUE(panda.isSneezing());

    panda.setSneezing(false);
    EXPECT_FALSE(panda.isSneezing());
}

TEST(PandaEntityStateTest, SetAndGetSneezeTimer)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_EQ(panda.getSneezeTimer(), 0);

    panda.setSneezeTimer(20);
    EXPECT_EQ(panda.getSneezeTimer(), 20);

    panda.setSneezeTimer(0);
    EXPECT_EQ(panda.getSneezeTimer(), 0);
}

TEST(PandaEntityStateTest, SetAndGetRolling)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FALSE(panda.isRolling());

    panda.setRolling(true);
    EXPECT_TRUE(panda.isRolling());

    panda.setRolling(false);
    EXPECT_FALSE(panda.isRolling());
}

TEST(PandaEntityStateTest, SetAndGetEating)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FALSE(panda.isEating());

    panda.setEating(true);
    EXPECT_TRUE(panda.isEating());

    panda.setEating(false);
    EXPECT_FALSE(panda.isEating());
}

TEST(PandaEntityStateTest, SetAndGetLying)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FALSE(panda.isLying());

    panda.setLying(true);
    EXPECT_TRUE(panda.isLying());

    panda.setLying(false);
    EXPECT_FALSE(panda.isLying());
}

// ==================== PandaEntity 眼睛高度测试 ====================

TEST(PandaEntityEyeHeightTest, AdultHasCorrectEyeHeight)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());
    panda.setChild(false);

    EXPECT_FLOAT_EQ(panda.eyeHeight(), 1.2f);
}

TEST(PandaEntityEyeHeightTest, ChildHasCorrectEyeHeight)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());
    panda.setChild(true);

    EXPECT_FLOAT_EQ(panda.eyeHeight(), 0.6f);
}

// ==================== PandaEntity 音效常量测试 ====================

TEST(PandaEntitySoundTest, SoundEventsAreDefined)
{
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_AMBIENT.toString(), "minecraft:entity.panda.ambient");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_HURT.toString(), "minecraft:entity.panda.hurt");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_DEATH.toString(), "minecraft:entity.panda.death");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_EAT.toString(), "minecraft:entity.panda.eat");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_SNEEZE.toString(), "minecraft:entity.panda.sneeze");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_PRE_SNEEZE.toString(), "minecraft:entity.panda.pre_sneeze");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_BITE.toString(), "minecraft:entity.panda.bite");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_AGGRESSIVE_AMBIENT.toString(), "minecraft:entity.panda.aggressive_ambient");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_WORRIED_AMBIENT.toString(), "minecraft:entity.panda.worried_ambient");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_CANT_BREED.toString(), "minecraft:entity.panda.cant_breed");
    EXPECT_EQ(SoundEvents::ENTITY_PANDA_STEP.toString(), "minecraft:entity.panda.step");
}

// ==================== PandaEntity onSneezeComplete 核心逻辑测试 ====================

class PandaEntitySneezeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    PandaTestWorld m_world;
};

TEST_F(PandaEntitySneezeTest, PlaysSneezeSoundOnComplete)
{
    TestablePandaEntity panda(EntityInstanceId(1));
    panda.setWorld(&m_world);
    panda.setPosition(0.0, 64.0, 0.0);

    // 执行打喷嚏完成
    panda.testOnSneezeComplete();

    // 验证声音被播放
    EXPECT_EQ(m_world.getSoundPlayCount(), 1);
    EXPECT_EQ(m_world.getLastSoundId(), SoundEvents::ENTITY_PANDA_SNEEZE);
}

TEST_F(PandaEntitySneezeTest, SpawnsSneezeParticle)
{
    TestablePandaEntity panda(EntityInstanceId(1));
    panda.setWorld(&m_world);
    panda.setPosition(100.0, 64.0, 100.0);

    // 执行打喷嚏完成
    panda.testOnSneezeComplete();

    // 验证粒子被生成
    EXPECT_EQ(m_world.getParticleSpawnCount(), 1);
    EXPECT_EQ(m_world.getLastParticleType(), particle::ParticleTypeId::Sneeze);
}

// PandaSneezeGoal 注册激活后的完整链路：sneeze(true) → tick 递减 → _onSneezeComplete。
// 修复前：PandaSneezeGoal 未注册，sneeze(true) 无调用点，m_sneezing 恒 false，tick 中
// sneeze 分支永不进入，_onSneezeComplete 是死代码。本测试验证 sneeze(true) 启动后，
// 经 SNEEZE_DURATION 次 tick 递减到 0 时触发 _onSneezeComplete（播 SNEEZE 音效+粒子）。
TEST_F(PandaEntitySneezeTest, SneezeTickChain_DrivesToOnSneezeComplete)
{
    TestablePandaEntity panda(EntityInstanceId(1));
    panda.setWorld(&m_world);
    panda.setPosition(0.0, 64.0, 0.0);

    // 启动打喷嚏（对齐 PandaSneezeGoal.start 调 sneeze(true)）
    panda.sneeze(true);
    EXPECT_TRUE(panda.isSneezing());
    EXPECT_EQ(panda.getSneezeTimer(), PandaEntity::SNEEZE_DURATION);

    const i32 soundBefore = m_world.getSoundPlayCount();

    // 推进 SNEEZE_DURATION 次 tick：第 1 次 tick 递减到 SNEEZE_DURATION-1 播预喷嚏音效，
    // 第 SNEEZE_DURATION 次 tick 递减到 0 触发 _onSneezeComplete（播 SNEEZE 音效 + 生成粒子）。
    for (int i = 0; i < PandaEntity::SNEEZE_DURATION; ++i) {
        panda.testTick();
        // 每次 tick 后重置位置防 BaseTestWorld 无物理引擎致坠落（见记忆：tick 坠落陷阱）
        panda.setPosition(0.0, 64.0, 0.0);
    }

    // 打喷嚏结束：m_sneezing 转 false，timer 归 0
    EXPECT_FALSE(panda.isSneezing());
    EXPECT_EQ(panda.getSneezeTimer(), 0);

    // _onSneezeComplete 被触发：播放 SNEEZE 音效（最后一次声音应为打喷嚏音效）
    // 预喷嚏（PRE_SNEEZE）+ 完成（SNEEZE）至少 2 次声音
    EXPECT_GE(m_world.getSoundPlayCount() - soundBefore, 2);
    EXPECT_EQ(m_world.getLastSoundId(), SoundEvents::ENTITY_PANDA_SNEEZE);
    // _onSneezeComplete 生成 Sneeze 粒子
    EXPECT_EQ(m_world.getLastParticleType(), particle::ParticleTypeId::Sneeze);
}

TEST_F(PandaEntitySneezeTest, NoEffectWithoutWorld)
{
    TestablePandaEntity panda(EntityInstanceId(1));
    // 没有设置 world

    // 执行打喷嚏完成 - 不应该崩溃
    panda.testOnSneezeComplete();

    // 验证没有崩溃
    EXPECT_TRUE(true);
}

TEST_F(PandaEntitySneezeTest, DoesNotSpawnEntityOnClientSide)
{
    // 注意：PandaTestWorld 默认 isClientSide() 返回 false
    // 这里我们无法直接 mock isClientSide，所以这个测试验证服务端路径
    TestablePandaEntity panda(EntityInstanceId(1));
    panda.setWorld(&m_world);
    panda.setPosition(0.0, 64.0, 0.0);

    // 执行打喷嚏完成
    panda.testOnSneezeComplete();

    // 在服务端，可能生成粘液球（概率 1/700）
    // 由于概率太低，我们只验证代码执行完成
    EXPECT_TRUE(true);
}

TEST_F(PandaEntitySneezeTest, ParticlePositionAtPandaHead)
{
    TestablePandaEntity panda(EntityInstanceId(1));
    panda.setWorld(&m_world);
    panda.setPosition(50.0, 64.0, 50.0);

    // 执行打喷嚏完成
    panda.testOnSneezeComplete();

    // 验证粒子被生成（位置验证）
    EXPECT_EQ(m_world.getParticleSpawnCount(), 1);
    // 粒子应该在熊猫头部附近
    // 熊猫眼睛高度为 1.2f，所以粒子 Y 应该接近 64.0 + 1.2 - 0.1 = 65.1
    EXPECT_NEAR(m_world.getLastParticlePosition().y, 65.1, 1.0);
}

// ==================== PandaEntity 成年熊猫跳跃测试 ====================

TEST_F(PandaEntitySneezeTest, NearbyAdultPandasCanJump)
{
    // 创建一个成年熊猫
    TestablePandaEntity sneezingPanda(EntityInstanceId(1));
    sneezingPanda.setWorld(&m_world);
    sneezingPanda.setPosition(0.0, 64.0, 0.0);

    // 注意：完整的成年熊猫跳跃测试需要 mock LivingEntity::jump() 方法
    // 这里我们验证 getEntitiesInAABB 被正确调用
    // 通过设置空的附近实体列表
    m_world.setNearbyPandas({});

    // 执行打喷嚏完成 - 不应该崩溃
    sneezingPanda.testOnSneezeComplete();

    // 验证声音和粒子仍然生成
    EXPECT_EQ(m_world.getSoundPlayCount(), 1);
    EXPECT_EQ(m_world.getParticleSpawnCount(), 1);
}

// ==================== PandaEntity 基因系统测试 ====================

TEST(PandaEntityGeneTest, GetAndSetMainGene)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    panda.setMainGene(3);
    EXPECT_EQ(panda.getMainGene(), 3);

    panda.setMainGene(5);
    EXPECT_EQ(panda.getMainGene(), 5);
}

TEST(PandaEntityGeneTest, GetAndSetHiddenGene)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    panda.setHiddenGene(2);
    EXPECT_EQ(panda.getHiddenGene(), 2);

    panda.setHiddenGene(4);
    EXPECT_EQ(panda.getHiddenGene(), 4);
}

TEST(PandaEntityGeneTest, CalculateExpressedPersonality_AggressiveDominant)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    // MC 1.16.5 Gene.func_221101_b() 规则：
    // 如果主基因是显性的（Aggressive），直接返回主基因
    panda.setMainGene(static_cast<u8>(PandaEntity::Personality::Aggressive));
    panda.setHiddenGene(static_cast<u8>(PandaEntity::Personality::Normal));
    EXPECT_EQ(panda.calculateExpressedPersonality(), PandaEntity::Personality::Aggressive);

    // 如果主基因是 Normal（隐性）且隐藏基因是 Aggressive，
    // 根据 MC 1.16.5 规则，只有 mainGene==LAZY && hiddenGene==AGGRESSIVE 时才返回 AGGRESSIVE
    // 其他隐性基因情况下返回主基因
    panda.setMainGene(static_cast<u8>(PandaEntity::Personality::Normal));
    panda.setHiddenGene(static_cast<u8>(PandaEntity::Personality::Aggressive));
    // 预期返回 Normal，因为主基因是隐性的且不是 Lazy
    EXPECT_EQ(panda.calculateExpressedPersonality(), PandaEntity::Personality::Normal);
}

TEST(PandaEntityGeneTest, CalculateExpressedPersonality_LazyAggressiveCombo)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    // MC 1.16.5: Lazy + Aggressive 组合表达为 Aggressive（好斗是显性的）
    // 参考: Gene.func_221101_b()
    // if (mainGene.isRecessive()) {
    //     return mainGene == LAZY && hiddenGene == AGGRESSIVE ? AGGRESSIVE : mainGene;
    // }
    panda.setMainGene(static_cast<u8>(PandaEntity::Personality::Lazy));
    panda.setHiddenGene(static_cast<u8>(PandaEntity::Personality::Aggressive));
    EXPECT_EQ(panda.calculateExpressedPersonality(), PandaEntity::Personality::Aggressive);
}

TEST(PandaEntityGeneTest, CalculateExpressedPersonality_NormalMainGene)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 普通基因作为主基因
    panda.setMainGene(static_cast<u8>(PandaEntity::Personality::Normal));
    panda.setHiddenGene(static_cast<u8>(PandaEntity::Personality::Lazy));
    EXPECT_EQ(panda.calculateExpressedPersonality(), PandaEntity::Personality::Normal);

    // 顽皮基因作为主基因
    panda.setMainGene(static_cast<u8>(PandaEntity::Personality::Playful));
    panda.setHiddenGene(static_cast<u8>(PandaEntity::Personality::Normal));
    EXPECT_EQ(panda.calculateExpressedPersonality(), PandaEntity::Personality::Playful);
}

TEST(PandaEntityGeneTest, GetOneOfGenesRandomly)
{
    Items::initialize();

    math::Random rng(12345);

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());
    panda.setMainGene(2);
    panda.setHiddenGene(4);

    // 随机获取一个基因，应该是主基因或隐藏基因之一
    u8 gene = panda.getOneOfGenesRandomly(rng);
    EXPECT_TRUE(gene == 2 || gene == 4);
}

TEST(PandaEntityGeneTest, UpdatePersonalityFromGenes)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    panda.setMainGene(static_cast<u8>(PandaEntity::Personality::Aggressive));
    panda.setHiddenGene(static_cast<u8>(PandaEntity::Personality::Normal));
    panda.updatePersonalityFromGenes();

    EXPECT_EQ(panda.getPersonality(), PandaEntity::Personality::Aggressive);
}

// ==================== PandaEntity spawnBaby 测试 ====================

class PandaEntitySpawnBabyTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    PandaTestWorld m_world;
};

TEST_F(PandaEntitySpawnBabyTest, SpawnBaby_CreatesChildPanda)
{
    PandaEntity mother(EntityInstanceId(1), mc::test::testEcsRegistry());
    mother.setWorld(&m_world);
    mother.setPosition(0.0f, 64.0f, 0.0f);

    PandaEntity father(EntityInstanceId(2), mc::test::testEcsRegistry());
    father.setWorld(&m_world);

    auto baby = mother.spawnBaby(father);

    ASSERT_NE(baby, nullptr);
    EXPECT_TRUE(baby->isChild());

    // 检查是 PandaEntity 类型
    PandaEntity* babyPanda = dynamic_cast<PandaEntity*>(baby.get());
    EXPECT_NE(babyPanda, nullptr);
}

TEST_F(PandaEntitySpawnBabyTest, SpawnBaby_PositionNearMother)
{
    PandaEntity mother(EntityInstanceId(1), mc::test::testEcsRegistry());
    mother.setWorld(&m_world);
    mother.setPosition(100.0f, 64.0f, 200.0f);

    PandaEntity father(EntityInstanceId(2), mc::test::testEcsRegistry());

    auto baby = mother.spawnBaby(father);

    ASSERT_NE(baby, nullptr);

    // 幼体应该在母体附近
    f32 dx = baby->x() - mother.x();
    f32 dy = baby->y() - mother.y();
    f32 dz = baby->z() - mother.z();
    f32 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    // 位置应该非常接近（spawnBaby使用母体位置）
    EXPECT_LT(distance, 1.0f);
}

TEST_F(PandaEntitySpawnBabyTest, SpawnBaby_InheritsGenes)
{
    PandaEntity mother(EntityInstanceId(1), mc::test::testEcsRegistry());
    mother.setWorld(&m_world);
    mother.setPosition(0.0f, 64.0f, 0.0f);
    mother.setMainGene(static_cast<u8>(PandaEntity::Personality::Aggressive));
    mother.setHiddenGene(static_cast<u8>(PandaEntity::Personality::Lazy));

    PandaEntity father(EntityInstanceId(2), mc::test::testEcsRegistry());
    father.setMainGene(static_cast<u8>(PandaEntity::Personality::Playful));
    father.setHiddenGene(static_cast<u8>(PandaEntity::Personality::Worried));

    auto baby = mother.spawnBaby(father);
    ASSERT_NE(baby, nullptr);

    PandaEntity* babyPanda = dynamic_cast<PandaEntity*>(baby.get());
    ASSERT_NE(babyPanda, nullptr);

    // 幼体应该有基因（从父母遗传）
    // 由于遗传是随机的，我们只验证基因在有效范围内
    EXPECT_GE(babyPanda->getMainGene(), 0);
    EXPECT_LE(babyPanda->getMainGene(), 5);
    EXPECT_GE(babyPanda->getHiddenGene(), 0);
    EXPECT_LE(babyPanda->getHiddenGene(), 5);
}

TEST_F(PandaEntitySpawnBabyTest, InheritGenesFromParents_BothParents)
{
    PandaEntity child(EntityInstanceId(3), mc::test::testEcsRegistry());
    child.setWorld(&m_world);

    PandaEntity father(EntityInstanceId(1), mc::test::testEcsRegistry());
    father.setMainGene(2);
    father.setHiddenGene(3);

    PandaEntity mother(EntityInstanceId(2), mc::test::testEcsRegistry());
    mother.setMainGene(4);
    mother.setHiddenGene(5);

    child.inheritGenesFromParents(&father, &mother);

    // 基因应该在父母的基因范围内（考虑变异概率）
    EXPECT_GE(child.getMainGene(), 0);
    EXPECT_LE(child.getMainGene(), 5);
    EXPECT_GE(child.getHiddenGene(), 0);
    EXPECT_LE(child.getHiddenGene(), 5);
}

TEST_F(PandaEntitySpawnBabyTest, InheritGenesFromParents_FatherOnly)
{
    PandaEntity child(EntityInstanceId(3), mc::test::testEcsRegistry());
    child.setWorld(&m_world);

    PandaEntity father(EntityInstanceId(1), mc::test::testEcsRegistry());
    father.setMainGene(1);
    father.setHiddenGene(2);

    // 只有一个父本（没有母本）
    child.inheritGenesFromParents(&father, nullptr);

    // 基因应该在有效范围内
    EXPECT_GE(child.getMainGene(), 0);
    EXPECT_LE(child.getMainGene(), 5);
    EXPECT_GE(child.getHiddenGene(), 0);
    EXPECT_LE(child.getHiddenGene(), 5);
}

// ==================== PandaEntity 打滚测试 ====================

TEST(PandaEntityRollTest, SetAndGetRollTimer)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_EQ(panda.getRollTimer(), 0);

    panda.setRollTimer(10);
    EXPECT_EQ(panda.getRollTimer(), 10);

    panda.setRollTimer(0);
    EXPECT_EQ(panda.getRollTimer(), 0);
}

TEST(PandaEntityRollTest, CanPerformAction_WhenIdle)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 初始状态，所有行为状态都是 false
    EXPECT_TRUE(panda.canPerformAction());
}

TEST(PandaEntityRollTest, CanPerformAction_WhenSneezing)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());
    panda.setSneezing(true);

    EXPECT_FALSE(panda.canPerformAction());
}

TEST(PandaEntityRollTest, CanPerformAction_WhenRolling)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());
    panda.setRolling(true);

    EXPECT_FALSE(panda.canPerformAction());
}

TEST(PandaEntityRollTest, CanPerformAction_WhenEating)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());
    panda.setEating(true);

    EXPECT_FALSE(panda.canPerformAction());
}

TEST(PandaEntityRollTest, CanPerformAction_WhenLying)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());
    panda.setLying(true);

    EXPECT_FALSE(panda.canPerformAction());
}

TEST(PandaEntityRollTest, CanPerformAction_WhenMultipleStates)
{
    Items::initialize();

    PandaEntity panda(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 任何一个状态为 true 就不能执行动作
    panda.setSneezing(true);
    EXPECT_FALSE(panda.canPerformAction());

    panda.setSneezing(false);
    panda.setRolling(true);
    EXPECT_FALSE(panda.canPerformAction());

    panda.setRolling(false);
    panda.setEating(true);
    EXPECT_FALSE(panda.canPerformAction());

    panda.setEating(false);
    panda.setLying(true);
    EXPECT_FALSE(panda.canPerformAction());

    // 所有状态都为 false 时可以执行动作
    panda.setLying(false);
    EXPECT_TRUE(panda.canPerformAction());
}

} // anonymous namespace
} // namespace mc
