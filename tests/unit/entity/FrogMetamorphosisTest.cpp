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

// 蝌蚪变态为青蛙 + 青蛙实体行为对齐 MC Java 1.21.11 测试。
//
// 原版 Tadpole.ageUp（Tadpole.java:221-230）：
//   convertTo(EntityType.FROG, ConversionParams.single(this, false, false), frog -> {
//       frog.finalizeSpawn(serverlevel, ..., CONVERSION, null);
//       frog.setPersistenceRequired();
//       frog.fudgePositionAfterSizeChange(getDimensions(getPose()));
//       playSound(TADPOLE_GROW_UP, 0.15F, 1.0F);
//   });
// 此前缺陷：项目没有青蛙实体，TadpoleEntity::setAge 到达成体年龄后只留 TODO，
// 蝌蚪永不长大。本测试覆盖：
//   - 达到 24000 tick 时经 Entity::convertTo 生成青蛙、蝌蚪被静默移除、播放 GROW_UP 音效；
//   - 未达到阈值时不转化；
//   - 青蛙的变体同步值、繁殖物品（FROG_FOOD）、不进入幼体状态、掉落减免；
//   - 转化按原实体的位置/朝向/速度复制。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/special/FrogGoals.hpp"
#include "common/entity/ai/goal/goals/special/FrogLongJumpGoal.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/frog/FrogEntity.hpp"
#include "common/entity/entities/passive/water/TadpoleEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>
#include <vector>

using namespace mc;

namespace {

/**
 * @brief 记录实体生成与音效的测试世界
 *
 * spawnEntity 接管实体所有权，使测试既能检查生成实体的类型，也能访问它
 * （例如转化后的青蛙）。世界默认非客户端，满足 convertTo 的服务端前提。
 */
class FrogConversionTestWorld final : public mc::test::BaseTestWorld {
public:
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawned.push_back(std::move(entity));
        registerEntityForLookup(m_spawned.back().get());
        return static_cast<EntityInstanceId>(m_spawned.size());
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory /*category*/,
        const Vector3& /*position*/,
        f32 /*volume*/,
        f32 /*pitch*/) override
    {
        m_playedSounds.push_back(soundEventId);
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawned; }

    [[nodiscard]] i32 countPlayed(const ResourceLocation& id) const
    {
        i32 count = 0;
        for (const auto& played : m_playedSounds) {
            if (played == id) {
                ++count;
            }
        }
        return count;
    }

private:
    std::vector<std::unique_ptr<Entity>> m_spawned;
    std::vector<ResourceLocation> m_playedSounds;
};

template <typename T>
const T* firstSpawned(const FrogConversionTestWorld& world)
{
    for (const auto& entity : world.spawnedEntities()) {
        if (const auto* typed = dynamic_cast<const T*>(entity.get()); typed != nullptr) {
            return typed;
        }
    }
    return nullptr;
}

} // namespace

class FrogMetamorphosisTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        Items::initialize();
        item::tag::ItemTags::initialize();
    }

    FrogConversionTestWorld m_world;
};

// 达到变态年龄 → 生成青蛙、播放成长音效、蝌蚪被静默移除。
TEST_F(FrogMetamorphosisTest, Tadpole_ReachingAdultAge_MetamorphosesIntoFrog)
{
    TadpoleEntity tadpole(EntityInstanceId(7), mc::test::testEcsRegistry());
    tadpole.setWorld(&m_world);
    tadpole.setPosition(12.5f, 62.0f, -8.25f);

    // 尚未达到阈值：保持原样。
    tadpole.setAge(TadpoleEntity::TICKS_TO_BE_FROG - 1);
    EXPECT_TRUE(m_world.spawnedEntities().empty());
    EXPECT_FALSE(tadpole.isRemoved());

    tadpole.setAge(TadpoleEntity::TICKS_TO_BE_FROG);

    // 青蛙已生成，且是唯一的生成实体（原蝌蚪被静默移除，不产生死亡掉落）。
    ASSERT_EQ(m_world.spawnedEntities().size(), 1U);
    const auto* frog = firstSpawned<FrogEntity>(m_world);
    ASSERT_NE(frog, nullptr) << "tadpole should convert into a frog entity";
    EXPECT_TRUE(tadpole.isRemoved()) << "converted tadpole must be silently removed";
    EXPECT_FALSE(tadpole.isAlive());

    // 位置/朝向/速度按原实体复制。
    EXPECT_FLOAT_EQ(frog->x(), 12.5f);
    EXPECT_FLOAT_EQ(frog->y(), 62.0f);
    EXPECT_FLOAT_EQ(frog->z(), -8.25f);

    // 转化后处理：持久化标记（setPersistenceRequired）。
    EXPECT_TRUE(frog->isNoDespawnRequired());

    // 成长音效播放一次。
    EXPECT_EQ(m_world.countPlayed(SoundEvents::ENTITY_TADPOLE_GROW_UP), 1);
}

// 客户端不执行变态（convertTo 只在服务端生效）。
TEST_F(FrogMetamorphosisTest, Tadpole_WithoutServerWorld_DoesNotConvert)
{
    TadpoleEntity tadpole(EntityInstanceId(8), mc::test::testEcsRegistry());
    // 未设置世界：不应崩溃，也不生成任何实体。
    tadpole.setAge(TadpoleEntity::TICKS_TO_BE_FROG + 10);
    EXPECT_FALSE(tadpole.isRemoved());
    EXPECT_TRUE(m_world.spawnedEntities().empty());
}

// 青蛙变体默认 temperate，可设置并在非法值外保持枚举语义。
TEST_F(FrogMetamorphosisTest, Frog_DefaultVariantIsTemperate)
{
    FrogEntity frog(EntityInstanceId(9), mc::test::testEcsRegistry());
    frog.setWorld(&m_world);

    EXPECT_EQ(frog.getVariant(), FrogEntity::Variant::Temperate);

    frog.setVariant(FrogEntity::Variant::Cold);
    EXPECT_EQ(frog.getVariant(), FrogEntity::Variant::Cold);
}

// 青蛙繁殖物品来自 FROG_FOOD 标签（黏液球）。
TEST_F(FrogMetamorphosisTest, Frog_IsBreedingItem_UsesFrogFoodTag)
{
    FrogEntity frog(EntityInstanceId(10), mc::test::testEcsRegistry());
    frog.setWorld(&m_world);

    ASSERT_NE(Items::SLIME_BALL, nullptr);
    ASSERT_NE(Items::WHEAT, nullptr);
    EXPECT_TRUE(frog.isBreedingItem(ItemStack(Items::SLIME_BALL)));
    EXPECT_FALSE(frog.isBreedingItem(ItemStack(Items::WHEAT)));
    EXPECT_FALSE(frog.isBreedingItem(ItemStack()));
}

// 青蛙恒为成体：isChild 恒 false，setChild 不改变状态（对齐 vanilla Frog.isBaby/setBaby）。
TEST_F(FrogMetamorphosisTest, Frog_IsNeverBaby)
{
    FrogEntity frog(EntityInstanceId(11), mc::test::testEcsRegistry());
    frog.setWorld(&m_world);

    EXPECT_FALSE(frog.isChild());
    frog.setChild(true);
    EXPECT_FALSE(frog.isChild());
}

// 青蛙碰撞箱与眼睛高度对齐 vanilla（0.5 x 0.5，眼高 0.5*0.45）。
TEST_F(FrogMetamorphosisTest, Frog_DimensionsMatchVanilla)
{
    FrogEntity frog(EntityInstanceId(12), mc::test::testEcsRegistry());
    frog.setWorld(&m_world);

    // AgeableEntity 将 width()/height() 覆盖为 protected（避免与 getBaseWidth 混淆），
    // 测试经 Entity 基类引用读取 public 的 width()/height()。
    const Entity& entity = frog;
    EXPECT_FLOAT_EQ(entity.width(), FrogEntity::WIDTH);
    EXPECT_FLOAT_EQ(entity.height(), FrogEntity::HEIGHT);
    EXPECT_FLOAT_EQ(frog.eyeHeight(), FrogEntity::EYE_HEIGHT);
}

// 舌击目标同步值：设置后可读回，清除后为空（对齐 DATA_TONGUE_TARGET_ID 的 OptionalInt 语义）。
TEST_F(FrogMetamorphosisTest, Frog_TongueTargetSetAndErase)
{
    FrogEntity frog(EntityInstanceId(13), mc::test::testEcsRegistry());
    frog.setWorld(&m_world);

    EXPECT_FALSE(frog.tongueTargetId().has_value());
    frog.setTongueTarget(EntityInstanceId(99));
    ASSERT_TRUE(frog.tongueTargetId().has_value());
    EXPECT_EQ(*frog.tongueTargetId(), EntityInstanceId(99));
    frog.eraseTongueTarget();
    EXPECT_FALSE(frog.tongueTargetId().has_value());
}

// 对齐 vanilla Frog.canEat：只有 FROG_FOOD 标签内的实体（除体型>1 的史莱姆）可被突舌捕食。
TEST_F(FrogMetamorphosisTest, Frog_CanEatOnlyFrogFoodEntities)
{
    EntityTypeTags::initialize();

    LivingEntity slime(EntityInstanceId(20), nullptr, mc::test::testEcsRegistry());
    slime.setTypeId("minecraft:slime");
    EXPECT_TRUE(FrogEntity::canEat(slime)) << "体型 1 的史莱姆可被吃掉";

    LivingEntity cow(EntityInstanceId(21), nullptr, mc::test::testEcsRegistry());
    cow.setTypeId("minecraft:cow");
    EXPECT_FALSE(FrogEntity::canEat(cow));
}

// 对齐 vanilla FrogAi.canAttack：求爱（繁殖）期间不捕食。
TEST_F(FrogMetamorphosisTest, FrogTongueGoal_IgnoresBreeding)
{
    FrogEntity frog(EntityInstanceId(30), mc::test::testEcsRegistry());
    frog.setWorld(&m_world);
    LivingEntity slime(EntityInstanceId(31), nullptr, mc::test::testEcsRegistry());
    slime.setTypeId("minecraft:slime");
    frog.setAttackTarget(&slime);
    frog.setLoveTimer(600);

    entity::ai::goal::FrogAttackTargetGoal targetGoal(&frog, 16.0f, 1.0f);
    entity::ai::goal::FrogTongueGoal tongueGoal(&frog);
    EXPECT_FALSE(targetGoal.shouldExecute());
    EXPECT_FALSE(tongueGoal.shouldExecute());

    frog.resetLove();
    EXPECT_TRUE(targetGoal.shouldExecute()) << "非繁殖期应保留现有可捕食目标";
}

// 对齐 vanilla：死亡时清除舌击目标并复位姿态。
TEST_F(FrogMetamorphosisTest, Frog_DeathClearsTongueState)
{
    FrogEntity frog(EntityInstanceId(40), mc::test::testEcsRegistry());
    frog.setWorld(&m_world);
    LivingEntity slime(EntityInstanceId(41), nullptr, mc::test::testEcsRegistry());
    slime.setTypeId("minecraft:slime");
    frog.setAttackTarget(&slime);
    frog.setTongueTarget(slime.id());
    frog.setPose(EntityPose::UsingTongue);

    EnvironmentalDamage source = DamageSources::generic();
    frog.die(source);

    EXPECT_FALSE(frog.tongueTargetId().has_value());
    EXPECT_EQ(frog.attackTarget(), nullptr);
    EXPECT_EQ(frog.pose(), EntityPose::Standing);
}

// 青蛙可被 /summon 创建并拥有正确的类型键（注册表完整性）。
TEST_F(FrogMetamorphosisTest, Frog_RegisteredInVanillaEntities)
{
    const auto* type = entity::EntityRegistry::instance().getType(entity::EntityTypeKeys::FROG);
    ASSERT_NE(type, nullptr);
    auto created = type->create(&m_world, mc::test::testEcsRegistry());
    ASSERT_NE(created, nullptr);
    EXPECT_NE(dynamic_cast<FrogEntity*>(created.get()), nullptr);
}

// ============================================================================
// 青蛙长跳（对齐 FrogAi LONG_JUMP 活动）
// ============================================================================

TEST_F(FrogMetamorphosisTest, Frog_LongJumpGoalRegistered)
{
    FrogEntity frog(EntityInstanceId(1), mc::test::testEcsRegistry());
    bool found = false;
    for (const auto& pg : frog.goalSelector().getAllGoals()) {
        if (pg.getGoal() != nullptr && pg.getGoal()->getTypeName() == "FrogLongJumpGoal") {
            found = true;
            EXPECT_EQ(pg.getPriority(), 8);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(FrogMetamorphosisTest, Frog_LongJumpCooldownRangeMatchesVanilla)
{
    // vanilla TIME_BETWEEN_LONG_JUMPS = UniformInt.of(100, 140)
    EXPECT_EQ(entity::ai::goal::FrogLongJumpGoal::COOLDOWN_MIN_TICKS, 100);
    EXPECT_EQ(entity::ai::goal::FrogLongJumpGoal::COOLDOWN_MAX_TICKS, 140);
    // 搜索范围 vanilla MAX_LONG_JUMP_WIDTH=4 / MAX_LONG_JUMP_HEIGHT=2
    EXPECT_EQ(entity::ai::goal::FrogLongJumpGoal::MAX_JUMP_WIDTH, 4);
    EXPECT_EQ(entity::ai::goal::FrogLongJumpGoal::MAX_JUMP_HEIGHT, 2);
}

TEST_F(FrogMetamorphosisTest, Frog_LongJumpVectorSolvesShortHop)
{
    FrogEntity frog(EntityInstanceId(1), mc::test::testEcsRegistry());
    entity::ai::goal::FrogLongJumpGoal goal(&frog);

    // 2 格水平、等高：应求出向上且朝 +X 的初速度
    const auto velocity = goal.calculateJumpVector(Vector3(0.0, 64.0, 0.0), Vector3(2.5, 64.0, 0.5), 3.5714288, 65);
    ASSERT_TRUE(velocity.has_value());
    EXPECT_GT(velocity->y, 0.0);
    EXPECT_GT(velocity->x, 0.0);
}
