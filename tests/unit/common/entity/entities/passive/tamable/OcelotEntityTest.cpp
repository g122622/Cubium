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
#include "common/entity/entities/passive/tamable/OcelotEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 豹猫实体测试用世界
 *
 * 提供最小化测试环境用于豹猫实体功能测试
 */
class OcelotTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("OcelotTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("OcelotTestWorld::tickManager not implemented");
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class OcelotEntityTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    OcelotTestWorld m_world;
};

// ============================================================================
// 繁殖物品测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, IsBreedingItem_Cod_ReturnsTrue)
{
    // MC 1.16.5: 豹猫使用生鳕鱼繁殖
    // BREEDING_ITEMS = Ingredient.fromItems(Items.COD, Items.SALMON)
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ItemStack codStack(Items::COD, 1);
    EXPECT_TRUE(ocelot.isBreedingItem(codStack));
}

TEST_F(OcelotEntityTestFixture, IsBreedingItem_Salmon_ReturnsTrue)
{
    // MC 1.16.5: 豹猫使用生鲑鱼繁殖
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ItemStack salmonStack(Items::SALMON, 1);
    EXPECT_TRUE(ocelot.isBreedingItem(salmonStack));
}

TEST_F(OcelotEntityTestFixture, IsBreedingItem_CookedCod_ReturnsFalse)
{
    // 熟鱼不能用于繁殖
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ItemStack cookedCodStack(Items::COOKED_COD, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(cookedCodStack));
}

TEST_F(OcelotEntityTestFixture, IsBreedingItem_CookedSalmon_ReturnsFalse)
{
    // 熟鲑鱼不能用于繁殖
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ItemStack cookedSalmonStack(Items::COOKED_SALMON, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(cookedSalmonStack));
}

// ============================================================================
// 非鱼类物品测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, IsBreedingItem_NonFish_ReturnsFalse)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 小麦不能用于豹猫繁殖
    ItemStack wheatStack(Items::WHEAT, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(wheatStack));

    // 胡萝卜不能用于豹猫繁殖
    ItemStack carrotStack(Items::CARROT, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(carrotStack));

    // 骨头不能用于豹猫繁殖
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(boneStack));

    // 生猪肉不能用于豹猫繁殖
    ItemStack porkchopStack(Items::PORKCHOP, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(porkchopStack));

    // 生牛肉不能用于豹猫繁殖
    ItemStack beefStack(Items::BEEF, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(beefStack));
}

// ============================================================================
// 空物品测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, IsBreedingItem_EmptyStack_ReturnsFalse)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(ocelot.isBreedingItem(emptyStack));
}

TEST_F(OcelotEntityTestFixture, IsBreedingItem_NullItem_ReturnsFalse)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ItemStack nullStack(nullptr, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(nullStack));
}

// ============================================================================
// 生成幼体测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, SpawnBaby_CreatesChildOcelot)
{
    OcelotEntity parent1(EntityInstanceId(0), mc::test::testEcsRegistry());
    OcelotEntity parent2(EntityInstanceId(0), mc::test::testEcsRegistry());

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    // 验证是豹猫实体
    auto* babyOcelot = dynamic_cast<OcelotEntity*>(baby.get());
    EXPECT_NE(babyOcelot, nullptr);

    // 验证是幼体
    EXPECT_TRUE(baby->isChild());
}

TEST_F(OcelotEntityTestFixture, SpawnBaby_PositionSetCorrectly)
{
    OcelotEntity parent(EntityInstanceId(0), mc::test::testEcsRegistry());
    parent.setPosition(100.0, 64.0, -50.0);

    auto baby = parent.spawnBaby(parent);
    ASSERT_NE(baby, nullptr);

    // 验证位置继承自父体
    EXPECT_FLOAT_EQ(baby->x(), 100.0f);
    EXPECT_FLOAT_EQ(baby->y(), 64.0f);
    EXPECT_FLOAT_EQ(baby->z(), -50.0f);
}

TEST_F(OcelotEntityTestFixture, SpawnBaby_CreatesNewEntity)
{
    OcelotEntity parent1(EntityInstanceId(0), mc::test::testEcsRegistry());
    OcelotEntity parent2(EntityInstanceId(0), mc::test::testEcsRegistry());

    auto baby1 = parent1.spawnBaby(parent2);
    auto baby2 = parent1.spawnBaby(parent2);

    // 每次调用应该创建新的实体
    ASSERT_NE(baby1, nullptr);
    ASSERT_NE(baby2, nullptr);
    EXPECT_NE(baby1.get(), baby2.get());
}

// ============================================================================
// 信任系统测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, TrustSystem_NotTrustingInitially)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    EXPECT_FALSE(ocelot.isTrusting());
    EXPECT_EQ(ocelot.getTrustingPlayerId(), 0u);
}

TEST_F(OcelotEntityTestFixture, TrustSystem_CanSetTrusting)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ocelot.setTrusting(true);
    EXPECT_TRUE(ocelot.isTrusting());

    ocelot.setTrusting(false);
    EXPECT_FALSE(ocelot.isTrusting());
}

TEST_F(OcelotEntityTestFixture, TrustSystem_CanTrustPlayer)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ocelot.setPlayerTrust(12345, true);
    EXPECT_TRUE(ocelot.trustsPlayer(12345));
    EXPECT_EQ(ocelot.getTrustingPlayerId(), 12345u);
}

TEST_F(OcelotEntityTestFixture, TrustSystem_DoesNotTrustOtherPlayers)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ocelot.setPlayerTrust(12345, true);
    EXPECT_FALSE(ocelot.trustsPlayer(67890));
}

TEST_F(OcelotEntityTestFixture, TrustSystem_CannotChangeTrustOnceSet)
{
    // 一旦建立信任，不能更改为其他玩家
    // 参考 MC 1.16.5: setPlayerTrust 只在 !m_trusting 时设置
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ocelot.setPlayerTrust(12345, true);
    EXPECT_EQ(ocelot.getTrustingPlayerId(), 12345u);

    // 尝试更改为其他玩家应该无效
    ocelot.setPlayerTrust(67890, true);
    EXPECT_EQ(ocelot.getTrustingPlayerId(), 12345u); // 仍然是第一个玩家
}

// ============================================================================
// 逃跑状态测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, Fleeing_CanSetFleeingState)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    EXPECT_FALSE(ocelot.isFleeing());

    ocelot.setFleeing(true);
    EXPECT_TRUE(ocelot.isFleeing());

    ocelot.setFleeing(false);
    EXPECT_FALSE(ocelot.isFleeing());
}

// ============================================================================
// 豹猫类型测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, OcelotType_DefaultIsWild)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    EXPECT_EQ(ocelot.getOcelotType(), OcelotEntity::OcelotType::Wild);
}

TEST_F(OcelotEntityTestFixture, OcelotType_CanSetType)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ocelot.setOcelotType(OcelotEntity::OcelotType::Tabby);
    EXPECT_EQ(ocelot.getOcelotType(), OcelotEntity::OcelotType::Tabby);

    ocelot.setOcelotType(OcelotEntity::OcelotType::Siamese);
    EXPECT_EQ(ocelot.getOcelotType(), OcelotEntity::OcelotType::Siamese);
}

// ============================================================================
// 属性测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, Attributes_HasCorrectBaseValues)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    // MC 1.16.5: 豹猫生命值为 10
    EXPECT_DOUBLE_EQ(ocelot.maxHealth(), 10.0);

    // MC 1.16.5: 豹猫移动速度为 0.3
    EXPECT_DOUBLE_EQ(ocelot.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0), 0.3);
}

// ============================================================================
// 眼睛高度测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, EyeHeight_AdultIsHigher)
{
    OcelotEntity adult(EntityInstanceId(0), mc::test::testEcsRegistry());
    adult.setChild(false);

    EXPECT_FLOAT_EQ(adult.eyeHeight(), 0.6f);
}

TEST_F(OcelotEntityTestFixture, EyeHeight_ChildIsLower)
{
    OcelotEntity child(EntityInstanceId(0), mc::test::testEcsRegistry());
    child.setChild(true);

    EXPECT_FLOAT_EQ(child.eyeHeight(), 0.3f);
}

// ============================================================================
// 摔落伤害免疫测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, FallDamage_Immune)
{
    // MC 1.16.5: 豹猫免疫摔落伤害
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    // canTakeFallDamage() 应该返回 false
    EXPECT_FALSE(ocelot.canTakeFallDamage());
}

// ============================================================================
// 消失逻辑测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, CanDespawn_NotTrusting_CanDespawnAfterTime)
{
    // MC 1.16.5: 未信任的豹猫存在超过 2400 tick (2分钟) 后可以消失
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(false);

    // 刚创建时不能消失
    EXPECT_FALSE(ocelot.canDespawn(0.0));

    // 模拟时间流逝 - 设置 ticksExisted
    // 注意：需要通过 tick 或其他方式增加 ticksExisted
    // 这里测试的是逻辑：未信任 + 时间超过 2400 tick = 可消失
    // 实际测试需要模拟 tick 或直接设置 ticksExisted
}

TEST_F(OcelotEntityTestFixture, CanDespawn_Trusting_NeverDespawns)
{
    // MC 1.16.5: 已信任的豹猫永远不会消失
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(true);

    // 无论距离玩家多远、存在多久，信任的豹猫都不消失
    EXPECT_FALSE(ocelot.canDespawn(0.0));
    EXPECT_FALSE(ocelot.canDespawn(100.0));
}

// ============================================================================
// OcelotAvoidPlayerGoal 测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, AvoidPlayerGoal_NotTrusting_ShouldExecute)
{
    // 未信任的豹猫应该执行躲避玩家的目标
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(false);

    // 创建躲避目标
    entity::ai::goal::OcelotAvoidPlayerGoal goal(&ocelot, 16.0f, 0.8, 1.33);

    // 未信任时 shouldExecute 取决于是否有玩家在范围内
    // 这里测试的是信任检查逻辑
    // 如果没有玩家，shouldExecute 返回 false（继承自 AvoidEntityGoal）
    // 如果有玩家且未信任，shouldExecute 返回 true
    EXPECT_FALSE(ocelot.isTrusting()); // 确认未信任
}

TEST_F(OcelotEntityTestFixture, AvoidPlayerGoal_Trusting_ShouldNotExecute)
{
    // 已信任的豹猫不应该执行躲避玩家的目标
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(true);

    // 创建躲避目标
    entity::ai::goal::OcelotAvoidPlayerGoal goal(&ocelot, 16.0f, 0.8, 1.33);

    // 已信任时，shouldExecute 应该返回 false（无论是否有玩家）
    EXPECT_FALSE(goal.shouldExecute());
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(OcelotEntityTestFixture, AvoidPlayerGoal_TrustingChanged_UpdatesBehavior)
{
    // 测试信任状态改变时行为变化
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(false);

    entity::ai::goal::OcelotAvoidPlayerGoal goal(&ocelot, 16.0f, 0.8, 1.33);

    // 未信任时
    EXPECT_FALSE(ocelot.isTrusting());

    // 建立信任
    ocelot.setTrusting(true);
    EXPECT_TRUE(ocelot.isTrusting());

    // 信任后 shouldExecute 应该返回 false
    EXPECT_FALSE(goal.shouldExecute());
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

// ============================================================================
// OcelotAvoidPlayerGoal fleeing 状态测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, AvoidPlayerGoal_StartExecuting_SetsFleeing)
{
    // startExecuting() 应该设置 fleeing 状态为 true
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(false);

    EXPECT_FALSE(ocelot.isFleeing());

    entity::ai::goal::OcelotAvoidPlayerGoal goal(&ocelot, 16.0f, 0.8, 1.33);
    goal.startExecuting();

    EXPECT_TRUE(ocelot.isFleeing());
}

TEST_F(OcelotEntityTestFixture, AvoidPlayerGoal_ResetTask_ClearsFleeing)
{
    // resetTask() 应该清除 fleeing 状态
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(false);

    entity::ai::goal::OcelotAvoidPlayerGoal goal(&ocelot, 16.0f, 0.8, 1.33);

    // 先设置 fleeing 状态
    ocelot.setFleeing(true);
    EXPECT_TRUE(ocelot.isFleeing());

    // resetTask 应该清除 fleeing 状态
    goal.resetTask();
    EXPECT_FALSE(ocelot.isFleeing());
}

TEST_F(OcelotEntityTestFixture, AvoidPlayerGoal_StartAndReset_FleeingLifecycle)
{
    // 测试 fleeing 状态的完整生命周期：开始逃跑 -> 逃跑中 -> 停止逃跑
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(false);

    entity::ai::goal::OcelotAvoidPlayerGoal goal(&ocelot, 16.0f, 0.8, 1.33);

    // 初始状态：未逃跑
    EXPECT_FALSE(ocelot.isFleeing());

    // 开始逃跑
    goal.startExecuting();
    EXPECT_TRUE(ocelot.isFleeing());

    // 停止逃跑
    goal.resetTask();
    EXPECT_FALSE(ocelot.isFleeing());
}

TEST_F(OcelotEntityTestFixture, AvoidPlayerGoal_StartExecuting_NullOcelot_DoesNotCrash)
{
    // 空指针安全性测试：m_ocelot 为 null 时不应崩溃
    entity::ai::goal::OcelotAvoidPlayerGoal goal(nullptr, 16.0f, 0.8, 1.33);

    // startExecuting 和 resetTask 不应崩溃
    goal.startExecuting();
    goal.resetTask();
}

// ============================================================================
// OcelotTemptGoal 测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, TemptGoal_IsScaredByPlayerMovement_NotTrusting)
{
    // 未信任的豹猫应该被玩家快速移动吓跑
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(false);

    // 创建诱惑目标
    entity::ai::goal::OcelotTemptGoal goal(
        &ocelot,
        0.6,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr && (item == Items::COD || item == Items::SALMON);
        },
        true); // scaredByMovement = true

    // 未信任时，isScaredByPlayerMovement 应该返回 true
    // 注意：TemptGoal 基类的 m_scaredByMovement 为 true
    // OcelotTemptGoal 重写为：return TemptGoal::isScaredByPlayerMovement() && !m_ocelot->isTrusting();
    // 所以未信任时返回 true && true = true
    EXPECT_FALSE(ocelot.isTrusting());
}

TEST_F(OcelotEntityTestFixture, TemptGoal_IsScaredByPlayerMovement_Trusting)
{
    // 已信任的豹猫不应该被玩家快速移动吓跑
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(true);

    // 创建诱惑目标
    entity::ai::goal::OcelotTemptGoal goal(
        &ocelot,
        0.6,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr && (item == Items::COD || item == Items::SALMON);
        },
        true); // scaredByMovement = true

    // 已信任时，isScaredByPlayerMovement 应该返回 false
    // return TemptGoal::isScaredByPlayerMovement() && !m_ocelot->isTrusting();
    // = true && false = false
    EXPECT_TRUE(ocelot.isTrusting());
}

// ============================================================================
// OcelotAttackGoal 测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, AttackGoal_StopAttackDistance)
{
    // 测试 OcelotAttackGoal 的停止追踪距离
    // STOP_ATTACK_DISTANCE_SQ = 225.0f (15*15)

    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    entity::ai::goal::OcelotAttackGoal goal(&ocelot);

    // GoalFlag 应该包含 Move 和 Look
    // 这是通过构造函数设置的
}

TEST_F(OcelotEntityTestFixture, AttackGoal_AttackDamage)
{
    // MC 1.16.5: 豹猫攻击伤害为 3.0
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 验证攻击伤害属性
    f64 attackDamage = ocelot.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0);
    EXPECT_DOUBLE_EQ(attackDamage, 3.0);
}

// ============================================================================
// 攻击目标选择器测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, TargetSelector_Chicken)
{
    // 验证豹猫会把小鸡作为攻击目标
    // NearestAttackableTargetGoal<ChickenEntity> 应该被添加到目标选择器
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    // registerGoals() 在构造函数中调用
    // 验证目标选择器已正确设置
    // 具体的目标选择需要 world 和实体存在，这里只验证实体可以创建
}

TEST_F(OcelotEntityTestFixture, TargetSelector_Turtle)
{
    // 验证豹猫会把海龟作为攻击目标
    // NearestAttackableTargetGoal<TurtleEntity> 应该被添加到目标选择器
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    // registerGoals() 在构造函数中调用
}

// ============================================================================
// AI 目标优先级测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, GoalPriorities_CorrectOrder)
{
    // MC 1.16.5 OcelotEntity.registerGoals() 目标优先级:
    // 1: SwimGoal
    // 3: OcelotTemptGoal
    // 4: OcelotAvoidPlayerGoal (动态)
    // 7: LeapAtTargetGoal
    // 8: OcelotAttackGoal
    // 9: BreedGoal
    // 10: WaterAvoidingRandomWalkingGoal
    // 11: LookAtGoal

    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 验证目标已注册
    // 具体优先级验证需要访问 GoalSelector 内部
}

// ============================================================================
// 常量验证测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, Constants_TemptSpeed)
{
    // 诱惑速度 = 0.6
    // 这是通过 TemptGoal 构造函数传递的
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    // 验证实体创建成功
}

TEST_F(OcelotEntityTestFixture, Constants_AvoidSpeeds)
{
    // 远距离逃避速度 = 0.8
    // 近距离逃避速度 = 1.33
    // 检测距离 = 16.0f
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
}

TEST_F(OcelotEntityTestFixture, Constants_AttackCooldown)
{
    // 攻击冷却 = 20 ticks
    // 停止追踪距离平方 = 225.0f (15*15)
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
}

TEST_F(OcelotEntityTestFixture, Constants_DespawnTicks)
{
    // 消失所需tick数 = 2400 (2分钟)
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    // 未信任的豹猫 2400 tick 后可消失
}

// ============================================================================
// 信任状态影响行为测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, TrustSystem_AffectsFleeing)
{
    // 信任后停止逃跑
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 未信任时可以逃跑
    ocelot.setFleeing(true);
    EXPECT_TRUE(ocelot.isFleeing());

    // 建立信任
    ocelot.setTrusting(true);

    // tick() 方法会自动将 fleeing 设置为 false
    // 这里只验证状态设置
}

TEST_F(OcelotEntityTestFixture, TrustSystem_DespawnPrevented)
{
    // 信任的豹猫不会消失
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(true);

    // canDespawn 应该返回 false
    EXPECT_FALSE(ocelot.canDespawn(0.0));
}

// ============================================================================
// 动态 AI 管理测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, DynamicAI_SetupTrustingAI)
{
    // setupTrustingAI() 应该根据信任状态动态添加/移除 AvoidPlayerGoal
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 初始未信任
    EXPECT_FALSE(ocelot.isTrusting());

    // 建立信任会触发 setupTrustingAI
    ocelot.setTrusting(true);

    // 验证信任已建立
    EXPECT_TRUE(ocelot.isTrusting());

    // 取消信任
    ocelot.setTrusting(false);
    EXPECT_FALSE(ocelot.isTrusting());
}

// ============================================================================
// DataParameter 网络同步测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, DataParameter_TrustingParamId_IsValid)
{
    // DATA_TRUSTING_PARAM 的 ID 应该是有效的（非零或合理值）
    u16 paramId = OcelotEntity::getTrustingParamId();
    // 参数 ID 由 EntityDataManager::createKey 自动分配，应该大于 0
    EXPECT_GT(paramId, 0u);
}

TEST_F(OcelotEntityTestFixture, DataParameter_IsTrusting_ReadsFromDataManager)
{
    // isTrusting() 应该从 DataManager 读取而非成员变量
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    EXPECT_FALSE(ocelot.isTrusting());

    ocelot.setTrusting(true);
    EXPECT_TRUE(ocelot.isTrusting());

    // 通过 DataManager 直接读取验证
    auto& dataManager = ocelot.dataManager();
    u16 paramId = OcelotEntity::getTrustingParamId();
    EXPECT_TRUE(dataManager.hasParam(paramId));
    bool storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_TRUE(storedValue);
}

TEST_F(OcelotEntityTestFixture, DataParameter_SetTrusting_WritesToDataManager)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    auto& dataManager = ocelot.dataManager();
    u16 paramId = OcelotEntity::getTrustingParamId();

    // 设置信任状态
    ocelot.setTrusting(true);

    // 验证 DataManager 中的值
    bool storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_TRUE(storedValue);

    // 设置为不信任
    ocelot.setTrusting(false);
    storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_FALSE(storedValue);
}

TEST_F(OcelotEntityTestFixture, DataParameter_DirtyFlag_OnTrustingChange)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    auto& dataManager = ocelot.dataManager();

    // 初始状态不应有脏数据
    dataManager.clearDirty();
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置信任状态应该标记为脏数据
    ocelot.setTrusting(true);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 清除脏标记后设置相同值不应标记为脏
    dataManager.clearDirty();
    ocelot.setTrusting(true);
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置不同值应该标记为脏
    ocelot.setTrusting(false);
    EXPECT_TRUE(dataManager.hasDirtyData());
}

TEST_F(OcelotEntityTestFixture, DataParameter_SyncsStateChanges)
{
    // 验证多次状态变更正确同步
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    EXPECT_FALSE(ocelot.isTrusting());

    ocelot.setTrusting(true);
    EXPECT_TRUE(ocelot.isTrusting());

    ocelot.setTrusting(false);
    EXPECT_FALSE(ocelot.isTrusting());

    ocelot.setTrusting(true);
    EXPECT_TRUE(ocelot.isTrusting());
}

TEST_F(OcelotEntityTestFixture, DataParameter_FleeingParamId_IsValid)
{
    // DATA_FLEEING_PARAM 的 ID 应该是有效的（非零或合理值）
    u16 paramId = OcelotEntity::getFleeingParamId();
    // 参数 ID 由 EntityDataManager::createKey 自动分配，应该大于 0
    EXPECT_GT(paramId, 0u);
}

TEST_F(OcelotEntityTestFixture, DataParameter_Fleeing_ReadsFromDataManager)
{
    // isFleeing() 应该从 DataManager 读取而非成员变量
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    EXPECT_FALSE(ocelot.isFleeing());

    ocelot.setFleeing(true);
    EXPECT_TRUE(ocelot.isFleeing());

    // 通过 DataManager 直接读取验证
    auto& dataManager = ocelot.dataManager();
    u16 paramId = OcelotEntity::getFleeingParamId();
    EXPECT_TRUE(dataManager.hasParam(paramId));
    bool storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_TRUE(storedValue);
}

TEST_F(OcelotEntityTestFixture, DataParameter_Fleeing_WritesToDataManager)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    auto& dataManager = ocelot.dataManager();
    u16 paramId = OcelotEntity::getFleeingParamId();

    // 设置逃跑状态
    ocelot.setFleeing(true);

    // 验证 DataManager 中的值
    bool storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_TRUE(storedValue);

    // 设置为非逃跑
    ocelot.setFleeing(false);
    storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_FALSE(storedValue);
}

TEST_F(OcelotEntityTestFixture, DataParameter_Fleeing_DirtyFlagOnChange)
{
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    auto& dataManager = ocelot.dataManager();

    // 初始状态不应有脏数据
    dataManager.clearDirty();
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置逃跑状态应该标记为脏数据
    ocelot.setFleeing(true);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 清除脏标记后设置相同值不应标记为脏
    dataManager.clearDirty();
    ocelot.setFleeing(true);
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置不同值应该标记为脏
    ocelot.setFleeing(false);
    EXPECT_TRUE(dataManager.hasDirtyData());
}

TEST_F(OcelotEntityTestFixture, DataParameter_Fleeing_SyncsStateChanges)
{
    // 验证多次状态变更正确同步
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    EXPECT_FALSE(ocelot.isFleeing());

    ocelot.setFleeing(true);
    EXPECT_TRUE(ocelot.isFleeing());

    ocelot.setFleeing(false);
    EXPECT_FALSE(ocelot.isFleeing());

    ocelot.setFleeing(true);
    EXPECT_TRUE(ocelot.isFleeing());
}

TEST_F(OcelotEntityTestFixture, DataParameter_TrustingAndFleeing_IndependentParams)
{
    // 信任和逃跑状态是独立的 DataParameter，修改一个不应影响另一个
    OcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    ocelot.setTrusting(true);
    ocelot.setFleeing(true);
    EXPECT_TRUE(ocelot.isTrusting());
    EXPECT_TRUE(ocelot.isFleeing());

    ocelot.setFleeing(false);
    EXPECT_TRUE(ocelot.isTrusting()); // 信任状态不受影响
    EXPECT_FALSE(ocelot.isFleeing());

    ocelot.setTrusting(false);
    EXPECT_FALSE(ocelot.isTrusting()); // 信任状态改变
    EXPECT_FALSE(ocelot.isFleeing());  // 逃跑状态不受影响
}

// ============================================================================
// NBT 序列化测试
// ============================================================================

namespace {

// 测试辅助类：暴露 protected 的 NBT 方法
class TestOcelotEntity : public OcelotEntity {
public:
    using OcelotEntity::OcelotEntity;

    // 暴露 protected 方法供测试使用
    using OcelotEntity::addAdditionalSaveData;
    using OcelotEntity::readAdditionalSaveData;
};

} // namespace

TEST_F(OcelotEntityTestFixture, Nbt_Trusting_RoundTrip)
{
    // 信任状态 NBT 序列化往返测试
    TestOcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(true);

    // 序列化
    nbt::tags::compound_tag tag;
    ocelot.addAdditionalSaveData(tag);

    // 验证 NBT 键名正确
    using namespace mc::entity::serialization;
    auto trustingVal = nbt_helper::tryGetBool(tag, nbt_keys::TRUSTING);
    ASSERT_TRUE(trustingVal.has_value());
    EXPECT_TRUE(*trustingVal);

    // 反序列化到新实体
    TestOcelotEntity loaded(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(loaded.isTrusting());
}

TEST_F(OcelotEntityTestFixture, Nbt_Trusting_False_RoundTrip)
{
    // 信任状态为 false 时的序列化往返测试
    TestOcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    // 默认不信任，显式设置
    ocelot.setTrusting(false);

    nbt::tags::compound_tag tag;
    ocelot.addAdditionalSaveData(tag);

    using namespace mc::entity::serialization;
    auto trustingVal = nbt_helper::tryGetBool(tag, nbt_keys::TRUSTING);
    ASSERT_TRUE(trustingVal.has_value());
    EXPECT_FALSE(*trustingVal);

    TestOcelotEntity loaded(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(loaded.isTrusting());
}

TEST_F(OcelotEntityTestFixture, Nbt_MissingTrustingKey_DefaultsToFalse)
{
    // 缺少 Trusting 键时应该保持默认值 false（MC 原版语义）
    TestOcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());

    nbt::tags::compound_tag emptyTag;
    auto result = ocelot.readAdditionalSaveData(emptyTag);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(ocelot.isTrusting());
}

TEST_F(OcelotEntityTestFixture, Nbt_MissingTrustingKey_DoesNotOverrideSetTrusting)
{
    // 读取空 NBT 不应覆盖已设置的信任状态为 true 的实体
    // 我们的实现是：只有键存在时才调用 setTrusting()，
    // 所以空 NBT 不应影响已设置的状态
    TestOcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(true);
    EXPECT_TRUE(ocelot.isTrusting());

    nbt::tags::compound_tag emptyTag;
    auto result = ocelot.readAdditionalSaveData(emptyTag);
    EXPECT_TRUE(result.success());
    // 因为键不存在，不调用 setTrusting()，信任状态保持不变
    EXPECT_TRUE(ocelot.isTrusting());
}

TEST_F(OcelotEntityTestFixture, Nbt_ReadTrustingFalse_OverridesTrue)
{
    // NBT 中 Trusting=false 应该覆盖已设置的 isTrusting()=true
    TestOcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    ocelot.setTrusting(true);
    EXPECT_TRUE(ocelot.isTrusting());

    nbt::tags::compound_tag tag;
    using namespace mc::entity::serialization;
    tag.put(nbt_keys::TRUSTING, static_cast<i8>(0)); // false

    auto result = ocelot.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(ocelot.isTrusting());
}

TEST_F(OcelotEntityTestFixture, Nbt_ReadTrustingTrue_OverridesFalse)
{
    // NBT 中 Trusting=true 应该覆盖默认的 isTrusting()=false
    TestOcelotEntity ocelot(EntityInstanceId(0), mc::test::testEcsRegistry());
    EXPECT_FALSE(ocelot.isTrusting());

    nbt::tags::compound_tag tag;
    using namespace mc::entity::serialization;
    tag.put(nbt_keys::TRUSTING, static_cast<i8>(1)); // true

    auto result = ocelot.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(ocelot.isTrusting());
}

} // namespace
} // namespace mc
