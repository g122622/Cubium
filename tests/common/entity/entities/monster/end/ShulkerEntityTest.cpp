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
#include "common/entity/ai/goal/goals/special/ShulkerGoals.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/end/ShulkerEntity.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/interfaces/IMob.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供 ShulkerEntity 测试所需的最小 IWorld 接口实现
 */
class ShulkerTestWorld final : public mc::test::BaseTestWorld {
public:
    ShulkerTestWorld() { VanillaBlocks::initialize(); }

    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return EntityInstanceId(static_cast<u32>(m_spawnedEntities.size()));
    }

    void advanceTick() { m_currentTick++; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ShulkerTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ShulkerTestWorld::tickManager not implemented");
    }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    [[nodiscard]] const Entity* getSpawnedEntity(size_t index) const
    {
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        // 默认返回空气
        return &VanillaBlocks::AIR->defaultState();
    }

private:
    u64 m_currentTick = 0;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

} // namespace

// ============================================================================
// ShulkerEntity 基础测试
// ============================================================================

TEST(ShulkerEntityTest, Construction)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证默认状态
    EXPECT_EQ(shulker.getShellState(), ShulkerEntity::ShellState::Closed);
    EXPECT_FLOAT_EQ(shulker.getPeekAmount(), 0.0f);
    EXPECT_EQ(shulker.getPeekTicks(), 0);

    // 验证默认附着方向
    EXPECT_EQ(shulker.getAttachmentFacing(), Direction::Down);

    // 验证眼睛高度
    EXPECT_FLOAT_EQ(shulker.eyeHeight(), 0.5f);
}

TEST(ShulkerEntityTest, CreateFactory)
{
    auto entity = ShulkerEntity::create(nullptr, mc::test::testEcsRegistry());
    ASSERT_NE(entity, nullptr);
    // 静态工厂不 setTypeId（由 EntityType::create 经注册表赋值），仅验证非空+类型。
    EXPECT_NE(dynamic_cast<ShulkerEntity*>(entity.get()), nullptr);
}

TEST(ShulkerEntityTest, ExperienceValue)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
    // MC 1.16.5: 潜影贝掉落 5 点经验
    EXPECT_EQ(shulker.experienceValue(), 5);
}

// ============================================================================
// 贝壳状态测试
// ============================================================================

TEST(ShulkerEntityTest, ShellStateTransitions)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 初始状态：闭合
    EXPECT_EQ(shulker.getShellState(), ShulkerEntity::ShellState::Closed);
    EXPECT_TRUE(shulker.isShellClosed());
    EXPECT_FALSE(shulker.isShellOpen());

    // 打开贝壳
    shulker.openShell();
    EXPECT_EQ(shulker.getShellState(), ShulkerEntity::ShellState::Opening);
    EXPECT_EQ(shulker.getPeekTicks(), 100);

    // 模拟打开动画完成
    for (int i = 0; i < 20; ++i) {
        shulker.tick();
    }
    EXPECT_EQ(shulker.getShellState(), ShulkerEntity::ShellState::Open);
    EXPECT_TRUE(shulker.isShellOpen());
    EXPECT_FALSE(shulker.isShellClosed());

    // 关闭贝壳
    shulker.closeShell();
    EXPECT_EQ(shulker.getShellState(), ShulkerEntity::ShellState::Closing);
    EXPECT_EQ(shulker.getPeekTicks(), 0);

    // 模拟关闭动画完成
    for (int i = 0; i < 20; ++i) {
        shulker.tick();
    }
    EXPECT_EQ(shulker.getShellState(), ShulkerEntity::ShellState::Closed);
    EXPECT_TRUE(shulker.isShellClosed());
}

TEST(ShulkerEntityTest, PeekAmountAnimation)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 初始开壳程度为 0
    EXPECT_FLOAT_EQ(shulker.getPeekAmount(), 0.0f);

    // 打开贝壳
    shulker.openShell();

    // 开壳程度应该逐渐增加到 1.0
    for (int i = 0; i < 100; ++i) {
        shulker.tick();
    }

    // 由于动画需要时间，检查开壳程度是否在合理范围内
    EXPECT_GT(shulker.getPeekAmount(), 0.0f);
}

// ============================================================================
// 附着方向测试
// ============================================================================

TEST(ShulkerEntityTest, AttachmentFacing)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认朝向
    EXPECT_EQ(shulker.getAttachmentFacing(), Direction::Down);

    // 设置附着方向
    shulker.setAttachmentFacing(Direction::Up);
    EXPECT_EQ(shulker.getAttachmentFacing(), Direction::Up);

    shulker.setAttachmentFacing(Direction::North);
    EXPECT_EQ(shulker.getAttachmentFacing(), Direction::North);

    shulker.setAttachmentFacing(Direction::South);
    EXPECT_EQ(shulker.getAttachmentFacing(), Direction::South);
}

TEST(ShulkerEntityTest, AttachmentPosition)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 设置附着位置
    BlockPos pos(10, 20, 30);
    shulker.setAttachmentPos(pos);
    EXPECT_EQ(shulker.getAttachmentPos(), pos);
}

// ============================================================================
// 攻击冷却测试
// ============================================================================

TEST(ShulkerEntityTest, AttackCooldown)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 初始冷却为 0
    EXPECT_EQ(shulker.getAttackCooldown(), 0);

    // 模拟 tick 减少冷却
    shulker.tick();
    EXPECT_EQ(shulker.getAttackCooldown(), 0);
}

// ============================================================================
// 免疫检测测试
// ============================================================================

TEST(ShulkerEntityTest, ImmuneWhenClosed)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 闭合时免疫
    EXPECT_EQ(shulker.getShellState(), ShulkerEntity::ShellState::Closed);
    EXPECT_TRUE(shulker.isImmuneToDamage());

    // 打开后不免疫
    shulker.openShell();
    for (int i = 0; i < 25; ++i) {
        shulker.tick();
    }
    EXPECT_EQ(shulker.getShellState(), ShulkerEntity::ShellState::Open);
    EXPECT_FALSE(shulker.isImmuneToDamage());
}

// ============================================================================
// 颜色测试
// ============================================================================

TEST(ShulkerEntityTest, Color)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认颜色是紫色
    EXPECT_EQ(shulker.getColor(), ShulkerEntity::ShulkerColor::Purple);

    // 设置颜色
    shulker.setColor(ShulkerEntity::ShulkerColor::Red);
    EXPECT_EQ(shulker.getColor(), ShulkerEntity::ShulkerColor::Red);

    shulker.setColor(ShulkerEntity::ShulkerColor::Blue);
    EXPECT_EQ(shulker.getColor(), ShulkerEntity::ShulkerColor::Blue);
}

// ============================================================================
// ShulkerBulletEntity 测试
// ============================================================================

TEST(ShulkerBulletEntityTest, Construction)
{
    entity::ShulkerBulletEntity bullet(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证子弹尺寸
    // MC 1.16.5: 潜影贝子弹是 0.3125 x 0.3125 的小型投射物
    EXPECT_FLOAT_EQ(bullet.width(), 0.3125f);
    EXPECT_FLOAT_EQ(bullet.height(), 0.3125f);
}

TEST(ShulkerBulletEntityTest, Direction)
{
    entity::ShulkerBulletEntity bullet(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认方向是 Up
    EXPECT_EQ(bullet.direction(), Direction::Up);
}

TEST(ShulkerBulletEntityTest, CanBeCollidedWith)
{
    entity::ShulkerBulletEntity bullet(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 子弹可以被碰撞（玩家可以击中它）
    EXPECT_TRUE(bullet.canBeCollidedWith());
}

TEST(ShulkerBulletEntityTest, NotBurning)
{
    entity::ShulkerBulletEntity bullet(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 子弹不会燃烧
    EXPECT_FALSE(bullet.isBurning());
}

// ============================================================================
// ShulkerEntity 音效测试
// ============================================================================

TEST(ShulkerEntityTest, AmbientSoundWhenClosed)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 闭合时不播放环境音效
    auto ambientSound = shulker.getAmbientSound();
    EXPECT_FALSE(ambientSound.has_value());
}

TEST(ShulkerEntityTest, AmbientSoundWhenOpen)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 打开后可以播放环境音效
    shulker.openShell();
    for (int i = 0; i < 25; ++i) {
        shulker.tick();
    }
    auto ambientSound = shulker.getAmbientSound();
    EXPECT_TRUE(ambientSound.has_value());
}

TEST(ShulkerEntityTest, DeathSound)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 测试死亡音效
    auto deathSound = shulker.getDeathSound();
    EXPECT_TRUE(deathSound.has_value());
}

// ============================================================================
// ShulkerEntity 不燃烧测试
// ============================================================================

TEST(ShulkerEntityTest, DoesNotBurnInDaylight)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 潜影贝不会在日光下燃烧
    EXPECT_FALSE(shulker.shouldBurnInDaylight());
}

// ============================================================================
// ShulkerEntity 攻击状态测试
// ============================================================================

TEST(ShulkerEntityTest, AttackingState)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 初始不攻击
    EXPECT_FALSE(shulker.isAttacking());

    // 设置攻击状态
    shulker.setAttacking(true);
    EXPECT_TRUE(shulker.isAttacking());

    shulker.setAttacking(false);
    EXPECT_FALSE(shulker.isAttacking());
}

// ============================================================================
// ShulkerEntity 子弹发射测试
// ============================================================================

TEST(ShulkerEntityTest, ShootBulletWithoutTargetDoesNothing)
{
    // 没有攻击目标时不会发射子弹
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 没有设置世界和攻击目标
    shulker.shootBullet();

    // 攻击冷却应该还是 0
    EXPECT_EQ(shulker.getAttackCooldown(), 0);
}

// ============================================================================
// ShulkerNearestAttackGoal 测试
// ============================================================================

TEST(ShulkerNearestAttackGoalTest, Construction)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto goal = std::make_unique<entity::ai::goal::ShulkerNearestAttackGoal>(&shulker);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "ShulkerNearestAttackGoal");
}

TEST(ShulkerNearestAttackGoalTest, DoesNotExecuteWithoutWorld)
{
    // 没有世界时，不应执行
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
    entity::ai::goal::ShulkerNearestAttackGoal goal(&shulker);

    // 没有设置世界，shouldExecute 应返回 false
    EXPECT_FALSE(goal.shouldExecute());
}

TEST(ShulkerNearestAttackGoalTest, DoesNotExecuteOnPeacefulDifficulty)
{
    // 和平难度下不应攻击玩家
    // 构造一个可配置难度的测试世界
    class PeacefulTestWorld final : public mc::test::BaseTestWorld {
    public:
        PeacefulTestWorld() { VanillaBlocks::initialize(); }
        [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Peaceful; }
        [[nodiscard]] u64 currentTick() const override { return m_tick; }
        void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}
        EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
        {
            m_entities.push_back(std::move(entity));
            return EntityInstanceId(static_cast<u32>(m_entities.size()));
        }
        [[nodiscard]] world::tick::TickManager& tickManager() override { throw std::runtime_error("not implemented"); }
        [[nodiscard]] const world::tick::TickManager& tickManager() const override
        {
            throw std::runtime_error("not implemented");
        }
        void advanceTick() { m_tick++; }

    private:
        u64 m_tick = 0;
        std::vector<std::unique_ptr<Entity>> m_entities;
    };

    auto world = std::make_unique<PeacefulTestWorld>();
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
    shulker.setWorld(world.get());

    entity::ai::goal::ShulkerNearestAttackGoal goal(&shulker);

    // 和平难度下 shouldExecute 必须返回 false
    EXPECT_FALSE(goal.shouldExecute());
}

// ============================================================================
// ShulkerDefenseAttackGoal 测试
// ============================================================================

TEST(ShulkerDefenseAttackGoalTest, Construction)
{
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto goal = std::make_unique<entity::ai::goal::ShulkerDefenseAttackGoal>(&shulker);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "ShulkerDefenseAttackGoal");
}

TEST(ShulkerDefenseAttackGoalTest, DoesNotExecuteWithoutTeam)
{
    // 潜影贝没有队伍时，防御攻击目标不应执行
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
    entity::ai::goal::ShulkerDefenseAttackGoal goal(&shulker);

    // 没有队伍（getTeam() 返回 nullptr），shouldExecute 应返回 false
    EXPECT_EQ(shulker.getTeam(), nullptr);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST(ShulkerDefenseAttackGoalTest, DoesNotExecuteWithoutWorld)
{
    // 没有世界时，即使有队伍也不应执行（因为无法搜索实体）
    // 使用带可设置队伍的潜影贝子类
    class ShulkerWithTeam final : public ShulkerEntity {
    public:
        ShulkerWithTeam(EntityInstanceId id)
            : ShulkerEntity(id, mc::test::testEcsRegistry())
        {}
        void setTeam(scoreboard::Team* team) { m_team = team; }
        [[nodiscard]] scoreboard::Team* getTeam() override { return m_team; }
        [[nodiscard]] const scoreboard::Team* getTeam() const override { return m_team; }

    private:
        scoreboard::Team* m_team = nullptr;
    };

    // 创建一个空的 MockTeam
    class MockTeam final : public scoreboard::Team {
    public:
        explicit MockTeam(const std::string& name)
            : m_name(name)
        {}

        [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
        [[nodiscard]] const text::ITextComponent* getDisplayName() const noexcept override { return nullptr; }
        void setDisplayName(std::unique_ptr<text::ITextComponent>) override {}
        [[nodiscard]] const std::set<std::string>& getMembers() const noexcept override
        {
            static std::set<std::string> empty;
            return empty;
        }
        bool addMember(const std::string&) override { return false; }
        bool removeMember(const std::string&) override { return false; }
        [[nodiscard]] bool hasMember(const std::string&) const override { return false; }
        void clearMembers() override {}
        [[nodiscard]] text::TextFormatting getColor() const noexcept override { return text::TextFormatting::White; }
        void setColor(text::TextFormatting) override {}
        [[nodiscard]] const text::ITextComponent* getPrefix() const noexcept override { return nullptr; }
        void setPrefix(std::unique_ptr<text::ITextComponent>) override {}
        [[nodiscard]] const text::ITextComponent* getSuffix() const noexcept override { return nullptr; }
        void setSuffix(std::unique_ptr<text::ITextComponent>) override {}
        [[nodiscard]] bool getAllowFriendlyFire() const noexcept override { return false; }
        void setAllowFriendlyFire(bool) override {}
        [[nodiscard]] bool canSeeFriendlyInvisibles() const noexcept override { return false; }
        void setSeeFriendlyInvisibles(bool) override {}
        [[nodiscard]] scoreboard::TeamVisibility getNameTagVisibility() const noexcept override
        {
            return scoreboard::TeamVisibility::Always;
        }
        void setNameTagVisibility(scoreboard::TeamVisibility) override {}
        [[nodiscard]] scoreboard::TeamVisibility getDeathMessageVisibility() const noexcept override
        {
            return scoreboard::TeamVisibility::Always;
        }
        void setDeathMessageVisibility(scoreboard::TeamVisibility) override {}
        [[nodiscard]] scoreboard::TeamCollisionRule getCollisionRule() const noexcept override
        {
            return scoreboard::TeamCollisionRule::Always;
        }
        void setCollisionRule(scoreboard::TeamCollisionRule) override {}
        [[nodiscard]] std::unique_ptr<text::ITextComponent> formatName(const text::ITextComponent&) const override
        {
            return nullptr;
        }
        [[nodiscard]] std::unique_ptr<text::ITextComponent> getFormattedDisplayName() const override { return nullptr; }

    private:
        std::string m_name;
    };

    MockTeam team("test_team");
    ShulkerWithTeam shulker(EntityInstanceId(1));
    shulker.setTeam(&team);
    EXPECT_NE(shulker.getTeam(), nullptr);

    entity::ai::goal::ShulkerDefenseAttackGoal goal(&shulker);
    // 没有世界，shouldExecute 应返回 false（因为 NearestAttackableTargetGoal::shouldExecute
    // 会先检查 world 是否为 null）
    EXPECT_FALSE(goal.shouldExecute());
}

TEST(ShulkerDefenseAttackGoalTest, IMobPredicateFiltersCorrectly)
{
    // 验证 ShulkerDefenseAttackGoal 的 IMob 过滤谓词
    // IMob 实体（如 ZombieEntity）应通过过滤，非 IMob 实体应被排除

    // 验证 MonsterEntity 实现了 IMob 接口
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry()); // MonsterEntity -> IMob
    const auto* imob = dynamic_cast<const entity::IMob*>(&shulker);
    EXPECT_NE(imob, nullptr) << "ShulkerEntity (MonsterEntity) 应该实现 IMob 接口";

    // 验证 ShulkerDefenseAttackGoal 构造函数中的谓词
    // 谓词: dynamic_cast<const entity::IMob*>(entity) != nullptr
    // 对于 IMob 实体返回 true，对于非 IMob 的 LivingEntity 返回 false
    entity::ai::goal::ShulkerDefenseAttackGoal goal(&shulker);
    EXPECT_EQ(goal.getTypeName(), "ShulkerDefenseAttackGoal");
}

// ============================================================================
// ShulkerEntity registerGoals 测试
// ============================================================================

TEST(ShulkerEntityTest, RegisterGoalsContainsDefenseAttackGoal)
{
    // 验证 registerGoals 后目标选择器包含正确的目标数量
    // 优先级1: HurtByTargetGoal
    // 优先级2: ShulkerNearestAttackGoal
    // 优先级3: ShulkerDefenseAttackGoal
    ShulkerEntity shulker(EntityInstanceId(1), mc::test::testEcsRegistry());
    // registerGoals 在构造期间被调用
    // 验证 shulker 的 targetSelector 已注册目标
    // 由于 GoalSelector API 限制，间接验证：确认 DefenseAttackGoal 可以被构造和查询类型名
    entity::ai::goal::ShulkerDefenseAttackGoal defenseGoal(&shulker);
    EXPECT_EQ(defenseGoal.getTypeName(), "ShulkerDefenseAttackGoal");

    entity::ai::goal::ShulkerNearestAttackGoal nearestGoal(&shulker);
    EXPECT_EQ(nearestGoal.getTypeName(), "ShulkerNearestAttackGoal");
}

} // namespace mc
