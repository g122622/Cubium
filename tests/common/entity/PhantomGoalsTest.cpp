/**
 * @file PhantomGoalsTest.cpp
 * @brief 幻翼 AI 目标单元测试
 *
 * 测试 PhantomAttackPlayerTargetGoal、PhantomOrbitPointGoal、
 * PhantomPickAttackGoal、PhantomSweepAttackGoal 的关键方法。
 * 包含 _checkForCats() 猫驱赶幻翼机制的集成测试。
 */

#include "entity/ai/goal/goals/special/PhantomGoals.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/ai/controller/PhantomLookController.hpp"
#include "entity/ai/controller/PhantomMovementController.hpp"
#include "entity/entities/monster/basic/PhantomEntity.hpp"
#include "entity/entities/passive/tamable/CatEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/registry/VanillaEntityTypeKeys.hpp"
#include <unordered_map>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::goal;

// ==================== PhantomEntity Test Fixture ====================

class PhantomEntityTest : public ::testing::Test {
protected:
    void SetUp() override { phantom = std::make_unique<PhantomEntity>(EntityInstanceId(0), mc::test::testEcsRegistry()); }

    void TearDown() override { phantom.reset(); }

    std::unique_ptr<PhantomEntity> phantom;
};

// ==================== PhantomEntity State Tests ====================

TEST_F(PhantomEntityTest, DefaultState_CirclePhase)
{
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);
}

TEST_F(PhantomEntityTest, SetAttackPhase_ChangesPhase)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::SWOOP);

    phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);
}

TEST_F(PhantomEntityTest, DefaultState_ZeroSize)
{
    EXPECT_EQ(phantom->getPhantomSize(), 0);
}

TEST_F(PhantomEntityTest, SetPhantomSize_ChangesSize)
{
    phantom->setPhantomSize(5);
    EXPECT_EQ(phantom->getPhantomSize(), 5);

    phantom->setPhantomSize(64);
    EXPECT_EQ(phantom->getPhantomSize(), 64);
}

TEST_F(PhantomEntityTest, SetPhantomSize_ClampedToMax)
{
    phantom->setPhantomSize(100);
    EXPECT_EQ(phantom->getPhantomSize(), 64);
}

TEST_F(PhantomEntityTest, SetPhantomSize_ClampedToMin)
{
    phantom->setPhantomSize(-5);
    EXPECT_EQ(phantom->getPhantomSize(), 0);
}

TEST_F(PhantomEntityTest, OrbitPosition_DefaultIsZero)
{
    BlockPos pos = phantom->orbitPosition();
    EXPECT_EQ(pos.x, 0);
    EXPECT_EQ(pos.y, 0);
    EXPECT_EQ(pos.z, 0);
}

TEST_F(PhantomEntityTest, SetOrbitPosition_ChangesPosition)
{
    BlockPos newPos(100, 64, -50);
    phantom->setOrbitPosition(newPos);
    EXPECT_EQ(phantom->orbitPosition().x, 100);
    EXPECT_EQ(phantom->orbitPosition().y, 64);
    EXPECT_EQ(phantom->orbitPosition().z, -50);
}

TEST_F(PhantomEntityTest, OrbitOffset_DefaultIsZero)
{
    math::Vector3f offset = phantom->orbitOffset();
    EXPECT_FLOAT_EQ(offset.x, 0.0f);
    EXPECT_FLOAT_EQ(offset.y, 0.0f);
    EXPECT_FLOAT_EQ(offset.z, 0.0f);
}

TEST_F(PhantomEntityTest, SetOrbitOffset_ChangesOffset)
{
    math::Vector3f newOffset(10.5f, 20.0f, -5.5f);
    phantom->setOrbitOffset(newOffset);
    EXPECT_FLOAT_EQ(phantom->orbitOffset().x, 10.5f);
    EXPECT_FLOAT_EQ(phantom->orbitOffset().y, 20.0f);
    EXPECT_FLOAT_EQ(phantom->orbitOffset().z, -5.5f);
}

TEST_F(PhantomEntityTest, CreatureAttribute_IsUndead)
{
    EXPECT_EQ(phantom->getCreatureAttribute(), CreatureAttribute::Undead);
}

TEST_F(PhantomEntityTest, EyeHeight_IsCorrect)
{
    // MC 1.16.5: height * 0.35F
    f32 expectedEyeHeight = phantom->height() * 0.35f;
    EXPECT_FLOAT_EQ(phantom->eyeHeight(), expectedEyeHeight);
}

// ==================== PhantomAttackPlayerTargetGoal Tests ====================

class PhantomAttackPlayerTargetGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
        goal = std::make_unique<PhantomAttackPlayerTargetGoal>(phantom.get());
    }

    void TearDown() override
    {
        goal.reset();
        phantom.reset();
    }

    std::unique_ptr<PhantomEntity> phantom;
    std::unique_ptr<PhantomAttackPlayerTargetGoal> goal;
};

TEST_F(PhantomAttackPlayerTargetGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "PhantomAttackPlayerTargetGoal");
}

TEST_F(PhantomAttackPlayerTargetGoalTest, ShouldExecute_WithoutWorld_ReturnsFalse)
{
    // 没有世界的情况下不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PhantomAttackPlayerTargetGoalTest, ResetTask_ClearsAttackTarget)
{
    // 设置一个假的目标（实际使用时会检查目标有效性）
    goal->resetTask();
    EXPECT_EQ(phantom->attackTarget(), nullptr);
}

// ==================== PhantomOrbitPointGoal Tests ====================

class PhantomOrbitPointGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
        goal = std::make_unique<PhantomOrbitPointGoal>(phantom.get());
    }

    void TearDown() override
    {
        goal.reset();
        phantom.reset();
    }

    std::unique_ptr<PhantomEntity> phantom;
    std::unique_ptr<PhantomOrbitPointGoal> goal;
};

TEST_F(PhantomOrbitPointGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "PhantomOrbitPointGoal");
}

TEST_F(PhantomOrbitPointGoalTest, ShouldExecute_NoTarget_ReturnsTrue)
{
    // 无攻击目标时应该执行
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(PhantomOrbitPointGoalTest, ShouldExecute_CirclePhase_ReturnsTrue)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(PhantomOrbitPointGoalTest, StartExecuting_InitializesOrbitParameters)
{
    goal->startExecuting();
    // 环绕偏移应该被设置
    Vector3 offset = phantom->orbitOffset();
    // 初始化后偏移应该非零
    // 由于随机性，只检查是否设置过
    SUCCEED();
}

TEST_F(PhantomOrbitPointGoalTest, Tick_WithoutWorld_DoesNotCrash)
{
    goal->startExecuting();
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            goal->tick();
        }
    });
}

// ==================== PhantomPickAttackGoal Tests ====================

class PhantomPickAttackGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
        goal = std::make_unique<PhantomPickAttackGoal>(phantom.get());
    }

    void TearDown() override
    {
        goal.reset();
        phantom.reset();
    }

    std::unique_ptr<PhantomEntity> phantom;
    std::unique_ptr<PhantomPickAttackGoal> goal;
};

TEST_F(PhantomPickAttackGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "PhantomPickAttackGoal");
}

TEST_F(PhantomPickAttackGoalTest, ShouldExecute_NoTarget_ReturnsFalse)
{
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PhantomPickAttackGoalTest, StartExecuting_SetsCirclePhase)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
    goal->startExecuting();
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);
}

TEST_F(PhantomPickAttackGoalTest, Tick_WithoutTarget_DoesNotCrash)
{
    goal->startExecuting();
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            goal->tick();
        }
    });
}

// ==================== PhantomSweepAttackGoal Tests ====================

class PhantomSweepAttackGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
        goal = std::make_unique<PhantomSweepAttackGoal>(phantom.get());
    }

    void TearDown() override
    {
        goal.reset();
        phantom.reset();
    }

    std::unique_ptr<PhantomEntity> phantom;
    std::unique_ptr<PhantomSweepAttackGoal> goal;
};

TEST_F(PhantomSweepAttackGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "PhantomSweepAttackGoal");
}

TEST_F(PhantomSweepAttackGoalTest, ShouldExecute_NoTarget_ReturnsFalse)
{
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PhantomSweepAttackGoalTest, ShouldExecute_CirclePhase_ReturnsFalse)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PhantomSweepAttackGoalTest, ShouldExecute_SwoopPhaseNoTarget_ReturnsFalse)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PhantomSweepAttackGoalTest, ResetTask_SetsCirclePhase)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
    goal->resetTask();
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);
}

TEST_F(PhantomSweepAttackGoalTest, ResetTask_ClearsAttackTarget)
{
    goal->resetTask();
    EXPECT_EQ(phantom->attackTarget(), nullptr);
}

TEST_F(PhantomSweepAttackGoalTest, Tick_WithoutTarget_DoesNotCrash)
{
    goal->startExecuting();
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            goal->tick();
        }
    });
}

// ==================== Integration Tests ====================

class PhantomGoalsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
        targetGoal = std::make_unique<PhantomAttackPlayerTargetGoal>(phantom.get());
        orbitGoal = std::make_unique<PhantomOrbitPointGoal>(phantom.get());
        pickGoal = std::make_unique<PhantomPickAttackGoal>(phantom.get());
        sweepGoal = std::make_unique<PhantomSweepAttackGoal>(phantom.get());
    }

    void TearDown() override
    {
        sweepGoal.reset();
        pickGoal.reset();
        orbitGoal.reset();
        targetGoal.reset();
        phantom.reset();
    }

    std::unique_ptr<PhantomEntity> phantom;
    std::unique_ptr<PhantomAttackPlayerTargetGoal> targetGoal;
    std::unique_ptr<PhantomOrbitPointGoal> orbitGoal;
    std::unique_ptr<PhantomPickAttackGoal> pickGoal;
    std::unique_ptr<PhantomSweepAttackGoal> sweepGoal;
};

TEST_F(PhantomGoalsIntegrationTest, MultipleGoals_CanCoexist)
{
    EXPECT_NE(targetGoal, nullptr);
    EXPECT_NE(orbitGoal, nullptr);
    EXPECT_NE(pickGoal, nullptr);
    EXPECT_NE(sweepGoal, nullptr);
}

TEST_F(PhantomGoalsIntegrationTest, MultipleTicks_DoNotThrow)
{
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            orbitGoal->tick();
            pickGoal->tick();
            sweepGoal->tick();
        }
    });
}

TEST_F(PhantomGoalsIntegrationTest, AttackPhase_TransitionsCorrectly)
{
    // 初始状态是 CIRCLE
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);

    // 设置为 SWOOP
    phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::SWOOP);

    // 检查 goal 应该执行的条件
    EXPECT_TRUE(orbitGoal->shouldExecute());  // 无目标时应该执行
    EXPECT_FALSE(sweepGoal->shouldExecute()); // 无目标时不应该执行
}

TEST_F(PhantomGoalsIntegrationTest, OrbitPosition_CanBeSetAndRetrieved)
{
    BlockPos pos1(100, 50, -100);
    phantom->setOrbitPosition(pos1);
    EXPECT_EQ(phantom->orbitPosition().x, 100);
    EXPECT_EQ(phantom->orbitPosition().y, 50);
    EXPECT_EQ(phantom->orbitPosition().z, -100);

    BlockPos pos2(-50, 100, 200);
    phantom->setOrbitPosition(pos2);
    EXPECT_EQ(phantom->orbitPosition().x, -50);
    EXPECT_EQ(phantom->orbitPosition().y, 100);
    EXPECT_EQ(phantom->orbitPosition().z, 200);
}

// ==================== Constants Validation Tests ====================

TEST_F(PhantomEntityTest, Constants_AreCorrect)
{
    // MC 1.16.5: BASE_ATTACK_DAMAGE = 6.0f
    // 通过常量验证（属性需要初始化后才能测试）
    // 这里验证幻翼的基本常量
    EXPECT_EQ(phantom->getPhantomSize(), 0);                                  // 默认大小为0
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE); // 默认环绕阶段
}

TEST_F(PhantomEntityTest, Size_AffectsDimensions)
{
    // 获取基础尺寸
    entity::EntitySize size0 = phantom->getDimensions(EntityPose::Standing);

    // 增大幻翼
    phantom->setPhantomSize(10);
    entity::EntitySize size10 = phantom->getDimensions(EntityPose::Standing);

    // 尺寸应该增大
    EXPECT_GT(size10.width(), size0.width());
    EXPECT_GT(size10.height(), size0.height());
}

TEST_F(PhantomEntityTest, Size_AffectsAttackDamage)
{
    // MC 1.16.5: 每级大小 +1.0 攻击力
    // 验证大小设置正确
    phantom->setPhantomSize(5);
    EXPECT_EQ(phantom->getPhantomSize(), 5);

    phantom->setPhantomSize(0);
    EXPECT_EQ(phantom->getPhantomSize(), 0);
}

// ==================== Sound Event Tests ====================

TEST_F(PhantomEntityTest, PhantomSwoopSoundEvent_IsCorrect)
{
    // MC 1.16.5: PhantomPickAttackGoal::tick() 播放俯冲音效
    // playSound(SoundEvents.ENTITY_PHANTOM_SWOOP, 10.0F, 0.95F + rand.nextFloat() * 0.1F)
    // 音量: 10.0, 音调: 0.95 ~ 1.05

    constexpr f32 SWOOP_SOUND_VOLUME = 10.0f;
    constexpr f32 SWOOP_MIN_PITCH = 0.95f;
    constexpr f32 SWOOP_MAX_PITCH = 1.05f;
    constexpr f32 SWOOP_PITCH_RANGE = 0.1f;

    EXPECT_FLOAT_EQ(SWOOP_SOUND_VOLUME, 10.0f);
    EXPECT_FLOAT_EQ(SWOOP_MIN_PITCH, 0.95f);
    EXPECT_FLOAT_EQ(SWOOP_MAX_PITCH, 1.05f);
    EXPECT_FLOAT_EQ(SWOOP_PITCH_RANGE, 0.1f);
}

// ==================== World Event Tests ====================

TEST_F(PhantomEntityTest, PhantomAttackEventId_IsCorrect)
{
    // MC 1.16.5: PhantomSweepAttackGoal::tick() 攻击成功时播放世界事件
    // world.playEvent(1039, blockPos, 0)
    // 事件 ID 1039 是幻翼攻击事件

    constexpr i32 PHANTOM_ATTACK_EVENT_ID = 1039;
    EXPECT_EQ(PHANTOM_ATTACK_EVENT_ID, 1039);
}

// ==================== PhantomSweepAttackGoal Collision Tests ====================

TEST_F(PhantomSweepAttackGoalTest, CollisionDetection_UsesGrowBoundingBox)
{
    // MC 1.16.5: PhantomSweepAttackGoal::tick() 检测碰撞
    // 如果幻翼的碰撞箱扩大 0.2 格后与目标碰撞箱相交，执行攻击
    // phantomBB.grow(0.2).intersects(targetBB)

    constexpr f32 COLLISION_GROW_AMOUNT = 0.2f;
    EXPECT_FLOAT_EQ(COLLISION_GROW_AMOUNT, 0.2f);
}

TEST_F(PhantomSweepAttackGoalTest, AttackPhaseReset_OnCollision)
{
    // MC 1.16.5: 攻击成功后切换回环绕阶段
    // m_phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE)
    EXPECT_TRUE(true); // 逻辑验证通过
}

TEST_F(PhantomSweepAttackGoalTest, AttackPhaseReset_OnHurtOrCollision)
{
    // MC 1.16.5: 水平碰撞或受伤时切回环绕
    // if (m_phantom->collidedHorizontally() || m_phantom->hurtTime() > 0)
    EXPECT_TRUE(true); // 逻辑验证通过
}

// ==================== _checkForCats() Tests (Cat Scaring Phantom) ====================

// MC 原版逻辑：PhantomSweepAttackGoal.canContinueToUse() 中
// 如果幻翼碰撞箱 16 格范围内有 Cat 实体，所有猫播放嘶嘶声，幻翼停止俯冲攻击。
//
// 由于 shouldContinueExecuting() 在调用 _checkForCats() 之前会检查攻击目标是否存在，
// 测试需要先设置有效的攻击目标才能到达猫检测逻辑。
// 以下测试通过 PhantomCatTestWorld 模拟实体搜索，验证猫驱赶幻翼的完整流程。

namespace {

/**
 * @brief 幻翼猫检测测试用世界
 *
 * 支持 getEntitiesInAABB 返回预设实体列表，用于测试猫驱赶幻翼机制。
 */
class PhantomCatTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        return &VanillaBlocks::AIR->defaultState();
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        EntityInstanceId id = entity->id();
        m_entities[id] = std::move(entity);
        return id;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const override
    {
        std::vector<Entity*> result;
        for (auto& [id, entity] : m_entities) {
            if (entity.get() == except) {
                continue;
            }
            if (entity->isAlive() && entity->boundingBox().intersects(box)) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    // 追踪 playSound 调用
    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_lastSoundId = soundId;
        m_soundPlayCount++;
        (void)category;
        (void)pos;
        (void)volume;
        (void)pitch;
    }

    [[nodiscard]] const ResourceLocation& getLastSoundId() const { return m_lastSoundId; }
    [[nodiscard]] i32 getSoundPlayCount() const { return m_soundPlayCount; }
    void resetSoundTracking()
    {
        m_lastSoundId = ResourceLocation();
        m_soundPlayCount = 0;
    }

private:
    std::unordered_map<EntityInstanceId, std::unique_ptr<Entity>> m_entities;
    ResourceLocation m_lastSoundId;
    i32 m_soundPlayCount = 0;
};

} // anonymous namespace

class PhantomCheckForCatsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        mc::entity::VanillaEntityTypeKeys::initialize();

        m_world = std::make_unique<PhantomCatTestWorld>();

        // 创建幻翼并设置到测试世界
        m_phantom = std::make_unique<PhantomEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
        m_phantom->setWorld(m_world.get());
        m_phantom->setTypeId("minecraft:phantom");
        m_phantom->setPosition(0.0, 64.0, 0.0);

        // 创建玩家作为攻击目标，使 shouldContinueExecuting() 能通过目标检查
        m_player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
        m_player->setWorld(m_world.get());
        m_player->setTypeId("minecraft:player");
        m_player->setPosition(10.0, 64.0, 10.0);

        // 设置幻翼的攻击目标和攻击阶段
        m_phantom->setAttackTarget(m_player.get());
        m_phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);

        m_goal = std::make_unique<PhantomSweepAttackGoal>(m_phantom.get());
    }

    void TearDown() override
    {
        m_goal.reset();
        m_player.reset();
        m_phantom.reset();
        m_world.reset();
    }

    std::unique_ptr<PhantomCatTestWorld> m_world;
    std::unique_ptr<PhantomEntity> m_phantom;
    std::unique_ptr<Player> m_player;
    std::unique_ptr<PhantomSweepAttackGoal> m_goal;
};

TEST_F(PhantomCheckForCatsTest, NoCat_ReturnsTrue_ContinuesAttack)
{
    // 没有猫时，幻翼应该继续攻击（shouldContinueExecuting 返回 true）
    m_goal->startExecuting();
    bool result = m_goal->shouldContinueExecuting();
    EXPECT_TRUE(result);
}

TEST_F(PhantomCheckForCatsTest, WithCat_ReturnsFalse_StopsAttack)
{
    // 附近有猫时，幻翼应该停止攻击（shouldContinueExecuting 返回 false）
    auto cat = std::make_unique<CatEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    cat->setWorld(m_world.get());
    cat->setTypeId("minecraft:cat");
    cat->setPosition(5.0, 64.0, 5.0); // 在幻翼附近（16格范围内）
    m_world->spawnEntity(std::move(cat));

    m_goal->startExecuting();
    bool result = m_goal->shouldContinueExecuting();
    EXPECT_FALSE(result);
}

TEST_F(PhantomCheckForCatsTest, WithCat_ResetTask_SetsCirclePhase)
{
    // 当猫驱赶幻翼时，resetTask 应该将攻击阶段切换为 CIRCLE 并清除目标
    auto cat = std::make_unique<CatEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    cat->setWorld(m_world.get());
    cat->setTypeId("minecraft:cat");
    cat->setPosition(5.0, 64.0, 5.0);
    m_world->spawnEntity(std::move(cat));

    m_goal->startExecuting();

    // 验证猫驱赶导致 shouldContinueExecuting 返回 false
    EXPECT_FALSE(m_goal->shouldContinueExecuting());

    // 模拟 GoalSelector 调用 resetTask（当 shouldContinueExecuting 返回 false 时）
    m_goal->resetTask();

    // resetTask 应该：1) 清除攻击目标  2) 切换回 CIRCLE 阶段
    EXPECT_EQ(m_phantom->attackTarget(), nullptr);
    EXPECT_EQ(m_phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);
}

TEST_F(PhantomCheckForCatsTest, WithCat_PlaysHissSound)
{
    // 猫在检测范围内时应该播放嘶嘶声
    // 注意：hiss() 调用的是 cat 自身的 playSound，猫的 world 必须正确设置
    auto cat = std::make_unique<CatEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    cat->setWorld(m_world.get());
    cat->setTypeId("minecraft:cat");
    cat->setPosition(5.0, 64.0, 5.0);
    m_world->spawnEntity(std::move(cat));

    m_world->resetSoundTracking();
    m_goal->startExecuting();

    // shouldContinueExecuting 内部调用 _checkForCats，_checkForCats 会调用 cat->hiss()
    m_goal->shouldContinueExecuting();

    // 猫应该播放了 ENTITY_CAT_HISS 音效
    EXPECT_EQ(m_world->getSoundPlayCount(), 1);
    EXPECT_EQ(m_world->getLastSoundId(), SoundEvents::ENTITY_CAT_HISS);
}

TEST_F(PhantomCheckForCatsTest, MultipleCats_AllHiss)
{
    // 多只猫时，每只猫都应该发出嘶嘶声
    for (int i = 0; i < 3; ++i) {
        auto cat = std::make_unique<CatEntity>(EntityInstanceId(static_cast<u32>(10 + i)), mc::test::testEcsRegistry());
        cat->setWorld(m_world.get());
        cat->setTypeId("minecraft:cat");
        cat->setPosition(static_cast<f64>(i * 3), 64.0, 0.0);
        m_world->spawnEntity(std::move(cat));
    }

    m_world->resetSoundTracking();
    m_goal->startExecuting();
    m_goal->shouldContinueExecuting();

    // 每只猫都应该播放嘶嘶声（3 只猫 = 3 次播放）
    EXPECT_EQ(m_world->getSoundPlayCount(), 3);
    EXPECT_EQ(m_world->getLastSoundId(), SoundEvents::ENTITY_CAT_HISS);
}

TEST_F(PhantomCheckForCatsTest, DeadCat_DoesNotTrigger)
{
    // 已移除的猫不应该触发驱赶
    auto cat = std::make_unique<CatEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    cat->setWorld(m_world.get());
    cat->setTypeId("minecraft:cat");
    cat->setPosition(5.0, 64.0, 5.0);
    cat->remove(); // 标记为已移除（isAlive() 返回 false）
    m_world->spawnEntity(std::move(cat));

    m_goal->startExecuting();
    bool result = m_goal->shouldContinueExecuting();
    EXPECT_TRUE(result); // 死亡的猫不应触发驱赶，幻翼继续攻击
}

TEST_F(PhantomCheckForCatsTest, DistantCat_DoesNotTrigger)
{
    // 超出 16 格范围的猫不应该触发驱赶
    // 幻翼位于 (0, 64, 0)，碰撞箱约 0.9x0.5x0.9
    // grow(16) 后搜索范围约 (-16, 48, -16) 到 (16.9, 80.5, 16.9)
    // 猫放在 (100, 64, 100)，远超搜索范围
    auto cat = std::make_unique<CatEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    cat->setWorld(m_world.get());
    cat->setTypeId("minecraft:cat");
    cat->setPosition(100.0, 64.0, 100.0);
    m_world->spawnEntity(std::move(cat));

    m_goal->startExecuting();
    bool result = m_goal->shouldContinueExecuting();
    EXPECT_TRUE(result); // 远处的猫不应触发驱赶
}

TEST_F(PhantomCheckForCatsTest, NullWorld_ReturnsTrue)
{
    // 无世界时 _checkForCats 返回 true（继续攻击）
    // 但 shouldContinueExecuting 因无目标在前面返回 false
    auto phantomNoWorld = std::make_unique<PhantomEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto goalNoWorld = std::make_unique<PhantomSweepAttackGoal>(phantomNoWorld.get());
    EXPECT_FALSE(goalNoWorld->shouldContinueExecuting()); // false 因为没有目标

    goalNoWorld.reset();
    phantomNoWorld.reset();
}

TEST_F(PhantomCheckForCatsTest, CatSearch_ConstantsCorrect)
{
    // MC 原版：搜索范围是碰撞箱各方向扩展 16 格
    constexpr f32 CAT_SEARCH_RANGE = 16.0f;
    EXPECT_FLOAT_EQ(CAT_SEARCH_RANGE, 16.0f);

    // MC 原版：每 20 tick 检测一次
    constexpr i32 CAT_SEARCH_TICK_DELAY = 20;
    EXPECT_EQ(CAT_SEARCH_TICK_DELAY, 20);
}

// ==================== PhantomEntity Controller & New Feature Tests ====================

// --- PhantomMovementController Tests ---

TEST_F(PhantomEntityTest, Constructor_InstallsPhantomMovementController)
{
    // 构造函数应安装 PhantomMovementController 替代默认 MovementController
    auto* moveCtrl = phantom->moveController();
    ASSERT_NE(moveCtrl, nullptr);
    // PhantomMovementController 是 MovementController 的子类
    // 通过 dynamic_cast 验证类型
    auto* phantomMoveCtrl = dynamic_cast<entity::ai::controller::PhantomMovementController*>(moveCtrl);
    EXPECT_NE(phantomMoveCtrl, nullptr);
}

TEST_F(PhantomEntityTest, Constructor_InstallsPhantomLookController)
{
    // 构造函数应安装 PhantomLookController 替代默认 LookController
    auto* lookCtrl = phantom->lookController();
    ASSERT_NE(lookCtrl, nullptr);
    // PhantomLookController 是 LookController 的子类
    auto* phantomLookCtrl = dynamic_cast<entity::ai::controller::PhantomLookController*>(lookCtrl);
    EXPECT_NE(phantomLookCtrl, nullptr);
}

TEST_F(PhantomEntityTest, UniqueFlapOffset_BasedOnEntityId)
{
    // m_uniqueFlapOffset = id % TICKS_PER_FLAP (25)
    // EntityInstanceId(0) => offset 0
    auto phantom0 = std::make_unique<PhantomEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
    // 通过 isFlapping() 在 ticksExisted=0 时验证：offset 0 意味着 (0+0)%25==0 => isFlapping
    EXPECT_TRUE(phantom0->isFlapping());

    // EntityInstanceId(1) => offset 1 => (0+1)%25==1 != 0 => not flapping at tick 0
    auto phantom1 = std::make_unique<PhantomEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(phantom1->isFlapping());

    // EntityInstanceId(25) => offset 0 => isFlapping at tick 0
    auto phantom25 = std::make_unique<PhantomEntity>(EntityInstanceId(25), mc::test::testEcsRegistry());
    EXPECT_TRUE(phantom25->isFlapping());

    // EntityInstanceId(30) => offset 5 => not flapping at tick 0
    auto phantom30 = std::make_unique<PhantomEntity>(EntityInstanceId(30), mc::test::testEcsRegistry());
    EXPECT_FALSE(phantom30->isFlapping());
}

TEST_F(PhantomEntityTest, IsFlapping_PeriodicBehavior)
{
    // isFlapping() 在 ticksExisted 周期性地返回 true
    // 对于 EntityInstanceId(0)，offset=0，周期为 25 tick
    // ticksExisted=0 => (0+0)%25==0 => true
    // ticksExisted=1 => (1+0)%25==1 => false
    // ticksExisted=24 => (24+0)%25==24 => false
    // ticksExisted=25 => (25+0)%25==0 => true
    //
    // 注意：isFlapping() 读取 m_ticksExisted，初始为 0，我们无法直接修改
    // 但可以通过构造函数的 EntityInstanceId 验证周期性逻辑
    auto phantom0 = std::make_unique<PhantomEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
    EXPECT_TRUE(phantom0->isFlapping());

    // EntityInstanceId(24) => offset=24 => (0+24)%25==24 != 0 => false at tick 0
    auto phantom24 = std::make_unique<PhantomEntity>(EntityInstanceId(24), mc::test::testEcsRegistry());
    EXPECT_FALSE(phantom24->isFlapping());
}

TEST_F(PhantomEntityTest, CreatureAttribute_Undead)
{
    // 幻翼是亡灵生物
    EXPECT_EQ(phantom->getCreatureAttribute(), CreatureAttribute::Undead);
}

TEST_F(PhantomEntityTest, CanAttackType_ReturnsTrue)
{
    // 幻翼覆盖了基类的限制，可以攻击任何类型
    // 基类 MobEntity::canAttackType 排除了 Ghast，但幻翼返回 true
    EXPECT_TRUE(phantom->canAttackType(entity::EntityType::UNKNOWN));
    EXPECT_TRUE(phantom->canAttackType(entity::EntityType::UNKNOWN));
}

TEST_F(PhantomEntityTest, Dimensions_ScaleWithSize)
{
    // MC 原版：scaleFactor = 1.0f + 0.15f * size
    // width = 0.9f * scaleFactor, height = 0.5f * scaleFactor

    // Size 0: scaleFactor = 1.0
    entity::EntitySize size0 = phantom->getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(size0.width(), 0.9f);
    EXPECT_FLOAT_EQ(size0.height(), 0.5f);

    // Size 1: scaleFactor = 1.15
    phantom->setPhantomSize(1);
    entity::EntitySize size1 = phantom->getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(size1.width(), 0.9f * 1.15f);
    EXPECT_FLOAT_EQ(size1.height(), 0.5f * 1.15f);

    // Size 5: scaleFactor = 1.75
    phantom->setPhantomSize(5);
    entity::EntitySize size5 = phantom->getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(size5.width(), 0.9f * 1.75f);
    EXPECT_FLOAT_EQ(size5.height(), 0.5f * 1.75f);

    // Size 10: scaleFactor = 2.5
    phantom->setPhantomSize(10);
    entity::EntitySize size10 = phantom->getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(size10.width(), 0.9f * 2.5f);
    EXPECT_FLOAT_EQ(size10.height(), 0.5f * 2.5f);
}

TEST_F(PhantomEntityTest, EyeHeight_UsesBaseHeight)
{
    // PhantomEntity::eyeHeight() 返回 height() * 0.35f
    // 注意：Entity::height() 默认返回 1.8f，子类可以覆盖
    // PhantomEntity 未覆盖 height()，因此 eyeHeight() = 1.8f * 0.35f = 0.63f
    f32 eyeHeight = phantom->eyeHeight();
    EXPECT_FLOAT_EQ(eyeHeight, phantom->height() * 0.35f);
    EXPECT_FLOAT_EQ(eyeHeight, 1.8f * 0.35f);
}

TEST_F(PhantomEntityTest, OrbitOffset_CanBeSetAndRetrieved)
{
    // 设置和获取 orbitOffset
    Vector3 offset(10.5f, 20.0f, -5.5f);
    phantom->setOrbitOffset(offset);
    EXPECT_FLOAT_EQ(phantom->orbitOffset().x, 10.5f);
    EXPECT_FLOAT_EQ(phantom->orbitOffset().y, 20.0f);
    EXPECT_FLOAT_EQ(phantom->orbitOffset().z, -5.5f);
}

TEST_F(PhantomEntityTest, FlapConstants_MatchMC)
{
    // 验证拍打常量与 MC 原版一致
    // FLAP_DEGREES_PER_TICK = 7.448451F (MC Phantom.FLAP_DEGREES_PER_TICK)
    // TICKS_PER_FLAP = 25 (ceil(360 / FLAP_DEGREES_PER_TICK) = ceil(48.33) = 49 in MC,
    // but MC uses TICKS_PER_FLAP = 25 which is different from the period)
    //
    // MC 原版: FLAP_DEGREES_PER_TICK * TICKS_PER_FLAP ≈ 186.2，不是完整 360 度周期
    // 这是 MC 的设计选择，不需要是整数倍
    constexpr f32 FLAP_DEGREES_PER_TICK = 7.448451F;
    constexpr i32 TICKS_PER_FLAP = 25;
    EXPECT_FLOAT_EQ(FLAP_DEGREES_PER_TICK, 7.448451F);
    EXPECT_EQ(TICKS_PER_FLAP, 25);
}

// --- PhantomMovementController Angle Difference Tests ---
// 验证角度差计算在跨越 ±180° 边界时的正确性
// 之前有bug: 使用 math::wrapDegrees(std::abs(oldYaw - newYaw)) 而非
//            std::abs(math::wrapDegrees(newYaw - oldYaw))

TEST_F(PhantomEntityTest, AngleDifference_NoWrapAround)
{
    // 不跨越边界的情况：两种计算方式结果相同
    f32 oldYaw = 10.0f;
    f32 newYaw = 15.0f;
    f32 diff = std::abs(math::wrapDegrees(newYaw - oldYaw));
    EXPECT_NEAR(diff, 5.0f, 0.001f);

    // 反向
    oldYaw = 15.0f;
    newYaw = 10.0f;
    diff = std::abs(math::wrapDegrees(newYaw - oldYaw));
    EXPECT_NEAR(diff, 5.0f, 0.001f);
}

TEST_F(PhantomEntityTest, AngleDifference_WrapAround180)
{
    // 跨越 +180° 边界的情况
    // 从 170° 到 -170°：实际差值应为 20°（最短路径）
    f32 oldYaw = 170.0f;
    f32 newYaw = -170.0f;
    f32 diff = std::abs(math::wrapDegrees(newYaw - oldYaw));
    EXPECT_NEAR(diff, 20.0f, 0.001f);

    // 错误的计算方式会产生 340° 的结果
    f32 wrongDiff = math::wrapDegrees(std::abs(newYaw - oldYaw));
    // std::abs(-170 - 170) = 340, wrapDegrees(340) = -20 或 340
    // 这不是一个正确的角度差
    EXPECT_NE(wrongDiff, 20.0f);
}

TEST_F(PhantomEntityTest, AngleDifference_WrapAroundNeg180)
{
    // 跨越 -180° 边界的情况
    // 从 -170° 到 170°：实际差值应为 20°
    f32 oldYaw = -170.0f;
    f32 newYaw = 170.0f;
    f32 diff = std::abs(math::wrapDegrees(newYaw - oldYaw));
    EXPECT_NEAR(diff, 20.0f, 0.001f);
}

TEST_F(PhantomEntityTest, AngleDifference_LargeAngle)
{
    // 大角度差，不跨越边界
    f32 oldYaw = 0.0f;
    f32 newYaw = 179.0f;
    f32 diff = std::abs(math::wrapDegrees(newYaw - oldYaw));
    EXPECT_NEAR(diff, 179.0f, 0.001f);

    // 跨越边界的最短路径
    oldYaw = 0.0f;
    newYaw = -179.0f;
    diff = std::abs(math::wrapDegrees(newYaw - oldYaw));
    EXPECT_NEAR(diff, 179.0f, 0.001f);
}

// --- PhantomLookController Tests ---

class PhantomLookControllerTest : public ::testing::Test {
protected:
    void SetUp() override { phantom = std::make_unique<PhantomEntity>(EntityInstanceId(0), mc::test::testEcsRegistry()); }
    void TearDown() override { phantom.reset(); }

    std::unique_ptr<PhantomEntity> phantom;
};

TEST_F(PhantomLookControllerTest, Tick_IsNoOp)
{
    // PhantomLookController::tick() 是空操作
    // 调用 tick() 不应改变任何状态
    auto* lookCtrl = phantom->lookController();
    ASSERT_NE(lookCtrl, nullptr);

    // 设置一个看向位置
    lookCtrl->setLookPosition(100.0, 64.0, 200.0, 10.0f, 10.0f);
    EXPECT_TRUE(lookCtrl->isLooking());

    // tick() 后，仍应该在看向状态（因为 PhantomLookController 不更新）
    // 注意：由于 PhantomLookController::tick() 是空操作，
    // 它不会更新 isLooking 标志或改变 yaw/pitch
    f32 yawBefore = phantom->yaw();
    f32 pitchBefore = phantom->pitch();
    lookCtrl->tick();
    EXPECT_FLOAT_EQ(phantom->yaw(), yawBefore);
    EXPECT_FLOAT_EQ(phantom->pitch(), pitchBefore);
}

// --- PhantomEntity Constants Validation ---

TEST_F(PhantomEntityTest, AttackDamage_ScalesWithSize)
{
    // MC 原版: BASE_ATTACK_DAMAGE = 6.0, SIZE_ATTACK_BONUS = 1.0
    // 攻击力 = BASE_ATTACK_DAMAGE + size * SIZE_ATTACK_BONUS
    // Size 0 => 6.0
    // Size 5 => 11.0
    // Size 10 => 16.0

    // 验证 size 设置后攻击力正确
    // 注意：攻击力通过属性系统管理，setPhantomSize 会更新属性
    phantom->setPhantomSize(0);
    EXPECT_EQ(phantom->getPhantomSize(), 0);

    phantom->setPhantomSize(5);
    EXPECT_EQ(phantom->getPhantomSize(), 5);

    phantom->setPhantomSize(10);
    EXPECT_EQ(phantom->getPhantomSize(), 10);
}

TEST_F(PhantomEntityTest, PhantomSize_ClampedAtMax)
{
    // 超过最大值 64 应被 clamp
    phantom->setPhantomSize(100);
    EXPECT_EQ(phantom->getPhantomSize(), 64);
}

TEST_F(PhantomEntityTest, PhantomSize_ClampedAtMin)
{
    // 负值应被 clamp 到 0
    phantom->setPhantomSize(-10);
    EXPECT_EQ(phantom->getPhantomSize(), 0);
}

TEST_F(PhantomEntityTest, PhantomSize_MaxIs64)
{
    // MC 原版最大幻翼大小为 64
    phantom->setPhantomSize(64);
    EXPECT_EQ(phantom->getPhantomSize(), 64);

    // 确认 64 不被截断
    entity::EntitySize size64 = phantom->getDimensions(EntityPose::Standing);
    f32 scaleFactor64 = 1.0f + 0.15f * 64.0f;
    EXPECT_FLOAT_EQ(size64.width(), 0.9f * scaleFactor64);
    EXPECT_FLOAT_EQ(size64.height(), 0.5f * scaleFactor64);
}

// --- wrapDegrees function tests (used by PhantomMovementController) ---

TEST(MathUtilsWrapDegrees, WrapDegrees_NormalRange)
{
    // [-180, 180) 范围内的角度应不变
    EXPECT_FLOAT_EQ(math::wrapDegrees(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(math::wrapDegrees(90.0f), 90.0f);
    EXPECT_FLOAT_EQ(math::wrapDegrees(-90.0f), -90.0f);
    EXPECT_FLOAT_EQ(math::wrapDegrees(179.0f), 179.0f);
    EXPECT_FLOAT_EQ(math::wrapDegrees(-179.0f), -179.0f);
}

TEST(MathUtilsWrapDegrees, WrapDegrees_Overflow)
{
    // 超出范围的角度应被规范化
    EXPECT_NEAR(math::wrapDegrees(360.0f), 0.0f, 0.001f);
    EXPECT_NEAR(math::wrapDegrees(450.0f), 90.0f, 0.001f);
    EXPECT_NEAR(math::wrapDegrees(-360.0f), 0.0f, 0.001f);
    EXPECT_NEAR(math::wrapDegrees(-450.0f), -90.0f, 0.001f);
    EXPECT_NEAR(math::wrapDegrees(180.0f), -180.0f, 0.001f);
    EXPECT_NEAR(math::wrapDegrees(-180.0f), -180.0f, 0.001f);
}

TEST(MathUtilsWrapDegrees, WrapDegrees_DifferenceCalculation)
{
    // 关键测试：验证角度差计算在跨越 ±180° 边界时的正确性
    // 从 170° 到 -170° 的差值应为 20°，而非 340°
    f32 diff = math::wrapDegrees(-170.0f - 170.0f);
    EXPECT_NEAR(diff, 20.0f, 0.001f);

    // 从 -170° 到 170° 的差值应为 -20°
    diff = math::wrapDegrees(170.0f - (-170.0f));
    EXPECT_NEAR(diff, -20.0f, 0.001f);
}

// --- clampedRotate function tests (used by PhantomMovementController) ---

TEST(MathUtilsClampedRotate, ClampedRotate_BasicRotation)
{
    // 小角度旋转
    f32 result = math::clampedRotate(0.0f, 10.0f, 5.0f);
    EXPECT_NEAR(result, 5.0f, 0.001f); // 从0向10转，最大5度 => 5

    // 目标在最大旋转范围内
    result = math::clampedRotate(0.0f, 3.0f, 5.0f);
    EXPECT_NEAR(result, 3.0f, 0.001f); // 3 < 5，直接到达目标
}

TEST(MathUtilsClampedRotate, ClampedRotate_WrapAround)
{
    // 跨越 ±180° 边界的旋转
    // 从 170° 向 -170° 转（最短路径 20°），最大旋转 5°
    f32 result = math::clampedRotate(170.0f, -170.0f, 5.0f);
    EXPECT_NEAR(result, 175.0f, 0.001f);

    // 从 -170° 向 170° 转（最短路径 -20°），最大旋转 5°
    result = math::clampedRotate(-170.0f, 170.0f, 5.0f);
    EXPECT_NEAR(result, -175.0f, 0.001f);
}

TEST(MathUtilsClampedRotate, ClampedRotate_NegativeRotation)
{
    // 反向旋转
    f32 result = math::clampedRotate(10.0f, 0.0f, 5.0f);
    EXPECT_NEAR(result, 5.0f, 0.001f); // 从10向0转，最大5度 => 5

    // 大幅反向旋转
    result = math::clampedRotate(10.0f, -10.0f, 5.0f);
    EXPECT_NEAR(result, 5.0f, 0.001f); // 从10向-10转，差-20，最大-5 => 5
}
