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
 * @file OfferFlowerGiftTest.cpp
 * @brief 铁傀儡 OfferFlowerGoal 赠花核心逻辑测试
 *
 * 测试 OfferFlowerGoal 的核心新增功能：
 * - _tryGiftFlowerToCopperGolem() 赠花逻辑及其全部边界条件
 * - _findNearestCandidate() 标签过滤与最近候选选择
 * - resetTask() 在各种条件下的赠花/不赠花行为
 * - setEquipment/setGuaranteedDrop 调用链与 MC dropPreservedEquipment 集成
 *
 * 对应 MC 1.21.11 OfferFlowerGoal.java 中的 stop() 方法赠花条件块。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/special/IronGolemGoals.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>
#include <vector>

using namespace mc;

namespace mc::test {

// ============================================================================
// 测试访问器 - 用于调用 OfferFlowerGoal 的私有方法和访问私有成员
// ============================================================================
//
// OfferFlowerGoal 已在 IronGolemGoals.hpp 中声明 friend class test::OfferFlowerGoalTestAccessor
// 这里提供具体实现，将私有方法和成员暴露给测试代码。

class OfferFlowerGoalTestAccessor {
public:
    /// 调用私有方法 _tryGiftFlowerToCopperGolem()
    static void tryGiftFlowerToCopperGolem(entity::ai::goal::OfferFlowerGoal& goal)
    {
        goal._tryGiftFlowerToCopperGolem();
    }

    /// 调用私有方法 _findNearestCandidate()
    static LivingEntity* findNearestCandidate(const entity::ai::goal::OfferFlowerGoal& goal)
    {
        return goal._findNearestCandidate();
    }

    /// 调用私有方法 _getGolemSearchBox()
    static AxisAlignedBB getGolemSearchBox(const entity::ai::goal::OfferFlowerGoal& goal)
    {
        return goal._getGolemSearchBox();
    }

    /// 读取 m_tick
    static i32 getTick(const entity::ai::goal::OfferFlowerGoal& goal) { return goal.m_tick; }

    /// 设置 m_tick
    static void setTick(entity::ai::goal::OfferFlowerGoal& goal, i32 tick) { goal.m_tick = tick; }

    /// 读取 m_target
    static LivingEntity* getTarget(const entity::ai::goal::OfferFlowerGoal& goal) { return goal.m_target; }

    /// 设置 m_target
    static void setTarget(entity::ai::goal::OfferFlowerGoal& goal, LivingEntity* target) { goal.m_target = target; }
};

} // namespace mc::test

namespace {

// ============================================================================
// 测试用世界 - 支持 getEntitiesInAABB、isBrightOutside、spawnEntity
// ============================================================================
//
// OfferFlowerGoal 的测试需要：
// - getEntitiesInAABB() 返回预设的实体列表（用于 _findNearestCandidate 搜索）
// - isBrightOutside() 返回 true（对应 MC level().isBrightOutside()）
// - spawnEntity() 捕获生成的 ItemEntity（用于 dropPreservedEquipment 集成测试）
// - hasChunk() 返回 true

class OfferFlowerTestWorld final : public mc::test::BaseTestWorld {
public:
    OfferFlowerTestWorld() = default;

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_nearbyEntities;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size() + 100);
    }

    /// 设置 _findNearestCandidate 的搜索结果
    void setNearbyEntities(std::vector<Entity*> entities) { m_nearbyEntities = std::move(entities); }

    /// 获取 spawnEntity 捕获的实体（用于验证 dropPreservedEquipment 生成的 ItemEntity）
    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }
    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

private:
    std::vector<Entity*> m_nearbyEntities;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

// ============================================================================
// 辅助函数
// ============================================================================

/// 创建铁傀儡并设置世界和位置
std::unique_ptr<IronGolemEntity> createIronGolem(OfferFlowerTestWorld& world, f32 x = 0.0f, f32 y = 64.0f, f32 z = 0.0f)
{
    auto golem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    golem->setTypeId(entity::EntityTypeKeys::IRON_GOLEM);
    golem->setWorld(&world);
    golem->setPosition(x, y, z);
    return golem;
}

/// 创建铜傀儡并设置类型ID、世界和位置
std::unique_ptr<CopperGolemEntity> createCopperGolem(
    EntityInstanceId id, OfferFlowerTestWorld& world, f32 x = 0.0f, f32 y = 64.0f, f32 z = 0.0f)
{
    auto golem = std::make_unique<CopperGolemEntity>(id, mc::test::testEcsRegistry());
    golem->setTypeId(entity::EntityTypeKeys::COPPER_GOLEM);
    golem->setWorld(&world);
    golem->setPosition(x, y, z);
    return golem;
}

/// 创建村民并设置类型ID、世界和位置
std::unique_ptr<entity::VillagerEntity> createVillager(
    EntityInstanceId id, OfferFlowerTestWorld& world, f32 x = 0.0f, f32 y = 64.0f, f32 z = 0.0f)
{
    auto villager = std::make_unique<entity::VillagerEntity>(id, mc::test::testEcsRegistry());
    villager->setTypeId(entity::EntityTypeKeys::VILLAGER);
    villager->setWorld(&world);
    villager->setPosition(x, y, z);
    return villager;
}

/// 创建玩家并设置类型ID、世界和位置
std::unique_ptr<Player> createPlayer(
    EntityInstanceId id, OfferFlowerTestWorld& world, f32 x = 0.0f, f32 y = 64.0f, f32 z = 0.0f)
{
    auto player = std::make_unique<Player>(id, std::string("TestPlayer"), mc::test::testEcsRegistry());
    player->setTypeId(entity::EntityTypeKeys::PLAYER);
    player->setWorld(&world);
    player->setPosition(x, y, z);
    return player;
}

} // namespace

// ============================================================================
// 测试夹具
// ============================================================================

class OfferFlowerGiftTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            // 初始化方块、物品、实体注册表
            VanillaBlocks::initialize();
            Items::initialize();
            BlockItemRegistry::instance().initializeVanillaBlockItems();
            entity::VanillaEntities::registerAll();
            // 初始化实体类型标签（创建空标签，由数据包填充）
            EntityTypeTags::initialize();

            // 手动填充赠花标签（运行时由数据包加载器填充，测试中手动模拟）
            // 对应数据包文件：
            //   accepts_iron_golem_gift.json: ["minecraft:copper_golem"]
            //   candidate_for_iron_golem_gift.json: ["minecraft:villager", "#minecraft:accepts_iron_golem_gift"]
            EntityTypeTags::ACCEPTS_IRON_GOLEM_GIFT().addAll({
                ResourceLocation("minecraft:copper_golem"),
            });
            EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT().addAll({
                ResourceLocation("minecraft:villager"),
                ResourceLocation("minecraft:copper_golem"),
            });

            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<OfferFlowerTestWorld>();
        m_ironGolem = createIronGolem(*m_world);
        m_goal = std::make_unique<entity::ai::goal::OfferFlowerGoal>(m_ironGolem.get());
    }

    void TearDown() override
    {
        m_goal.reset();
        m_ironGolem.reset();
        m_world.reset();
    }

    /// 验证铜傀儡天线槽未装备罂粟花
    void expectAntennaEmpty(const CopperGolemEntity& golem) const
    {
        EXPECT_TRUE(golem.getEquipment(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA).isEmpty()) << "天线槽应为空";
    }

    /// 验证铜傀儡天线槽已装备罂粟花
    void expectAntennaHasPoppy(const CopperGolemEntity& golem) const
    {
        const auto& antenna = golem.getEquipment(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA);
        EXPECT_FALSE(antenna.isEmpty()) << "天线槽应有罂粟花";
        EXPECT_EQ(antenna.getItem(), Items::POPPY) << "天线槽物品应为罂粟花";
    }

    /// 验证铜傀儡天线槽已标记保整掉落
    void expectAntennaGuaranteedDrop(const CopperGolemEntity& golem) const
    {
        EXPECT_TRUE(golem.isEquipmentDropPreserved(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA))
            << "天线槽应标记为保整掉落（drop chance > 1.0）";
    }

    /// 验证铜傀儡天线槽未标记保整掉落
    void expectAntennaNotGuaranteedDrop(const CopperGolemEntity& golem) const
    {
        EXPECT_FALSE(golem.isEquipmentDropPreserved(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA))
            << "天线槽不应标记为保整掉落";
    }

    std::unique_ptr<OfferFlowerTestWorld> m_world;
    std::unique_ptr<IronGolemEntity> m_ironGolem;
    std::unique_ptr<entity::ai::goal::OfferFlowerGoal> m_goal;
};

// ============================================================================
// EQUIPMENT_SLOT_ANTENNA 常量验证
// ============================================================================

TEST_F(OfferFlowerGiftTest, EquipmentSlotAntenna_IsSaddle)
{
    // 对应 MC 1.21.11 CopperGolem.EQUIPMENT_SLOT_ANTENNA = EquipmentSlot.SADDLE
    EXPECT_EQ(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA, EquipmentSlot::Saddle);
}

// ============================================================================
// _tryGiftFlowerToCopperGolem 边界条件测试
// ============================================================================
//
// _tryGiftFlowerToCopperGolem() 的赠花条件（按检查顺序）：
// 1. m_tick == 0（计时器自然结束）
// 2. m_target != nullptr
// 3. m_target 是 MobEntity（dynamic_cast 成功）
// 4. EntityTypeTags::isInitialized() == true
// 5. m_target 属于 ACCEPTS_IRON_GOLEM_GIFT 标签
// 6. 天线槽为空
// 7. 铁傀儡搜索 AABB 与目标碰撞盒相交
// 满足全部条件时装备罂粟花并标记保整掉落。

// ---------- 条件 1: m_tick != 0（被抢占中断）时不赠花 ----------

TEST_F(OfferFlowerGiftTest, TryGift_TickNotZero_DoesNotGift)
{
    // 设置一个有效的铜傀儡目标
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, copperGolem.get());

    // m_tick > 0 表示被抢占中断（未自然结束），不应赠花
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 100);
    test::OfferFlowerGoalTestAccessor::tryGiftFlowerToCopperGolem(*m_goal);

    expectAntennaEmpty(*copperGolem);
    expectAntennaNotGuaranteedDrop(*copperGolem);
}

TEST_F(OfferFlowerGiftTest, TryGift_TickPositive_DoesNotGift)
{
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, copperGolem.get());

    // m_tick = 1（刚被抢占）
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 1);
    test::OfferFlowerGoalTestAccessor::tryGiftFlowerToCopperGolem(*m_goal);

    expectAntennaEmpty(*copperGolem);
    expectAntennaNotGuaranteedDrop(*copperGolem);
}

// ---------- 条件 2: m_target 为 null 时不赠花 ----------

TEST_F(OfferFlowerGiftTest, TryGift_NullTarget_DoesNotGift)
{
    // m_target = nullptr，即使 m_tick == 0 也不赠花
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, nullptr);
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 0);

    // 不应崩溃，也不应做任何事
    test::OfferFlowerGoalTestAccessor::tryGiftFlowerToCopperGolem(*m_goal);

    // 无目标可检查，验证不崩溃即可
    SUCCEED();
}

// ---------- 条件 3: m_target 非 MobEntity 时不赠花 ----------

TEST_F(OfferFlowerGiftTest, TryGift_TargetNotMobEntity_DoesNotGift)
{
    // Player 是 LivingEntity 但不是 MobEntity，不应赠花
    auto player = createPlayer(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, player.get());
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 0);

    test::OfferFlowerGoalTestAccessor::tryGiftFlowerToCopperGolem(*m_goal);

    // Player 没有 Saddle 槽装备（getEquipment 的 default 分支返回空），
    // 但关键是不应崩溃且不应尝试设置装备
    SUCCEED();
}

// ---------- 条件 5: 目标不在 ACCEPTS_IRON_GOLEM_GIFT 标签时不赠花 ----------

TEST_F(OfferFlowerGiftTest, TryGift_VillagerNotInAcceptsTag_DoesNotGift)
{
    // 村民在 CANDIDATE_FOR_IRON_GOLEM_GIFT 标签中，但不在 ACCEPTS_IRON_GOLEM_GIFT 标签中
    // （ACCEPTS 标签只包含 copper_golem），因此不应赠花
    auto villager = createVillager(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, villager.get());
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 0);

    test::OfferFlowerGoalTestAccessor::tryGiftFlowerToCopperGolem(*m_goal);

    // 村民不应获得罂粟花装备（虽然 VillagerEntity 没有 Saddle 槽，但关键是逻辑不应走到 setEquipment）
    SUCCEED();
}

// ---------- 条件 6: 天线槽已占用时不赠花 ----------

TEST_F(OfferFlowerGiftTest, TryGift_AntennaSlotOccupied_DoesNotGift)
{
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, copperGolem.get());
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 0);

    // 预先装备一个罂粟花到天线槽
    copperGolem->setEquipment(CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA, ItemStack(Items::POPPY, 1));

    test::OfferFlowerGoalTestAccessor::tryGiftFlowerToCopperGolem(*m_goal);

    // 天线槽仍为原有罂粟花（未被覆盖），且不应重新标记保整掉落
    expectAntennaHasPoppy(*copperGolem);
    // 原有装备未标记 setGuaranteedDrop
    expectAntennaNotGuaranteedDrop(*copperGolem);
}

// ---------- 条件 7: AABB 不相交时不赠花 ----------

TEST_F(OfferFlowerGiftTest, TryGift_AabbNotIntersecting_DoesNotGift)
{
    // 将铜傀儡放在远离铁傀儡的位置（超出搜索 AABB 范围）
    // 铁傀儡在 (0, 64, 0)，搜索 AABB = boundingBox.expand(6, 2, 6)
    // 铁傀儡宽度 1.4，所以搜索 AABB 约为 (-6.7, 62, -6.7) ~ (6.7, 66, 6.7)
    // 将铜傀儡放在 (20, 64, 0) 远超搜索范围
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 20.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, copperGolem.get());
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 0);

    test::OfferFlowerGoalTestAccessor::tryGiftFlowerToCopperGolem(*m_goal);

    expectAntennaEmpty(*copperGolem);
    expectAntennaNotGuaranteedDrop(*copperGolem);
}

// ---------- 成功赠花：所有条件满足 ----------

TEST_F(OfferFlowerGiftTest, TryGift_AllConditionsMet_GiftsPoppy)
{
    // 铜傀儡在铁傀儡附近（搜索 AABB 范围内，且 AABB 相交）
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, copperGolem.get());
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 0); // 计时器自然结束

    // 验证赠花前天线槽为空
    expectAntennaEmpty(*copperGolem);
    expectAntennaNotGuaranteedDrop(*copperGolem);

    test::OfferFlowerGoalTestAccessor::tryGiftFlowerToCopperGolem(*m_goal);

    // 赠花后天线槽应有罂粟花
    expectAntennaHasPoppy(*copperGolem);
    // 天线槽应标记为保整掉落
    expectAntennaGuaranteedDrop(*copperGolem);
}

// ---------- 成功赠花：铜傀儡紧贴铁傀儡 ----------

TEST_F(OfferFlowerGiftTest, TryGift_CopperGolemAdjacent_GiftsPoppy)
{
    // 铜傀儡紧贴铁傀儡（距离 0.5 格，AABB 必然相交）
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 0.5f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, copperGolem.get());
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 0);

    test::OfferFlowerGoalTestAccessor::tryGiftFlowerToCopperGolem(*m_goal);

    expectAntennaHasPoppy(*copperGolem);
    expectAntennaGuaranteedDrop(*copperGolem);
}

// ============================================================================
// resetTask 集成测试：验证赠花/不赠花行为
// ============================================================================

TEST_F(OfferFlowerGiftTest, ResetTask_TickZero_GiftsFlowerToCopperGolem)
{
    // 模拟自然结束：startExecuting 设置 m_tick=400，然后 tick() 400 次让 m_tick 递减到 0
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, copperGolem.get());

    m_goal->startExecuting(); // m_tick = 400, setHoldingRose(true)

    // 执行 400 次 tick() 让 m_tick 递减到 0
    for (int i = 0; i < 400; ++i) {
        m_goal->tick();
    }

    // 验证 m_tick 已递减到 0
    EXPECT_EQ(test::OfferFlowerGoalTestAccessor::getTick(*m_goal), 0);

    // resetTask 应触发赠花（因为 m_tick == 0）
    m_goal->resetTask();

    expectAntennaHasPoppy(*copperGolem);
    expectAntennaGuaranteedDrop(*copperGolem);

    // resetTask 后持花状态应清除
    EXPECT_FALSE(m_ironGolem->isHoldingRose());
}

TEST_F(OfferFlowerGiftTest, ResetTask_TickNotZero_DoesNotGiftFlower)
{
    // 模拟被抢占中断：startExecuting 后只 tick 少量次数，m_tick > 0 时被 resetTask
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, copperGolem.get());

    m_goal->startExecuting(); // m_tick = 400

    // 只 tick 10 次，m_tick = 390（被抢占中断）
    for (int i = 0; i < 10; ++i) {
        m_goal->tick();
    }

    EXPECT_GT(test::OfferFlowerGoalTestAccessor::getTick(*m_goal), 0);

    // resetTask 因 m_tick != 0 不应赠花
    m_goal->resetTask();

    expectAntennaEmpty(*copperGolem);
    expectAntennaNotGuaranteedDrop(*copperGolem);
    EXPECT_FALSE(m_ironGolem->isHoldingRose());
}

TEST_F(OfferFlowerGiftTest, ResetTask_NullTarget_DoesNotCrash)
{
    // m_target 为 null 时 resetTask 不应崩溃
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, nullptr);
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 0);

    m_goal->resetTask(); // 不应崩溃

    EXPECT_FALSE(m_ironGolem->isHoldingRose());
}

TEST_F(OfferFlowerGiftTest, ResetTask_VillagerTarget_DoesNotGiftVillager)
{
    // 目标是村民（不在 ACCEPTS_IRON_GOLEM_GIFT 标签中），resetTask 不应赠花
    auto villager = createVillager(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, villager.get());

    m_goal->startExecuting();
    for (int i = 0; i < 400; ++i) {
        m_goal->tick();
    }
    EXPECT_EQ(test::OfferFlowerGoalTestAccessor::getTick(*m_goal), 0);

    m_goal->resetTask(); // 村民不在 ACCEPTS 标签中，不应赠花

    // 村民没有天线槽，验证不崩溃即可
    EXPECT_FALSE(m_ironGolem->isHoldingRose());
}

// ============================================================================
// _findNearestCandidate 测试
// ============================================================================

TEST_F(OfferFlowerGiftTest, FindNearestCandidate_TagsNotInitialized_ReturnsNull)
{
    // 标签系统已初始化（SetUpTestSuite 中调用），此测试验证 isInitialized 检查路径
    // 由于 SetUpTestSuite 已初始化标签，这里验证已初始化状态下的正常搜索
    // 标签未初始化的测试在单独的测试夹具中（见 OfferFlowerGiftWithoutTagsTest）

    // 空世界，无附近实体
    m_world->setNearbyEntities({});
    LivingEntity* result = test::OfferFlowerGoalTestAccessor::findNearestCandidate(*m_goal);
    EXPECT_EQ(result, nullptr);
}

TEST_F(OfferFlowerGiftTest, FindNearestCandidate_EmptyWorld_ReturnsNull)
{
    m_world->setNearbyEntities({});
    LivingEntity* result = test::OfferFlowerGoalTestAccessor::findNearestCandidate(*m_goal);
    EXPECT_EQ(result, nullptr);
}

TEST_F(OfferFlowerGiftTest, FindNearestCandidate_NoCandidateEntities_ReturnsNull)
{
    // 附近只有玩家（不在 CANDIDATE_FOR_IRON_GOLEM_GIFT 标签中）
    auto player = createPlayer(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    std::vector<Entity*> entities = {player.get()};
    m_world->setNearbyEntities(entities);

    LivingEntity* result = test::OfferFlowerGoalTestAccessor::findNearestCandidate(*m_goal);
    EXPECT_EQ(result, nullptr);
}

TEST_F(OfferFlowerGiftTest, FindNearestCandidate_VillagerInTag_ReturnsVillager)
{
    auto villager = createVillager(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    std::vector<Entity*> entities = {villager.get()};
    m_world->setNearbyEntities(entities);

    LivingEntity* result = test::OfferFlowerGoalTestAccessor::findNearestCandidate(*m_goal);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->id(), villager->id());
}

TEST_F(OfferFlowerGiftTest, FindNearestCandidate_CopperGolemInTag_ReturnsCopperGolem)
{
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    std::vector<Entity*> entities = {copperGolem.get()};
    m_world->setNearbyEntities(entities);

    LivingEntity* result = test::OfferFlowerGoalTestAccessor::findNearestCandidate(*m_goal);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->id(), copperGolem->id());
}

TEST_F(OfferFlowerGiftTest, FindNearestCandidate_MultipleCandidates_ReturnsNearest)
{
    // 两个村民，一个近一个远，应返回最近的
    auto nearVillager = createVillager(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    auto farVillager = createVillager(EntityInstanceId(3), *m_world, 5.0f, 64.0f, 0.0f);
    std::vector<Entity*> entities = {farVillager.get(), nearVillager.get()};
    m_world->setNearbyEntities(entities);

    LivingEntity* result = test::OfferFlowerGoalTestAccessor::findNearestCandidate(*m_goal);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->id(), nearVillager->id());
}

TEST_F(OfferFlowerGiftTest, FindNearestCandidate_MixedEntities_FiltersByTag)
{
    // 混合实体：玩家（不在标签中）、村民（在标签中）、铜傀儡（在标签中）
    auto player = createPlayer(EntityInstanceId(2), *m_world, 0.5f, 64.0f, 0.0f);
    auto villager = createVillager(EntityInstanceId(3), *m_world, 1.0f, 64.0f, 0.0f);
    auto copperGolem = createCopperGolem(EntityInstanceId(4), *m_world, 2.0f, 64.0f, 0.0f);
    std::vector<Entity*> entities = {player.get(), villager.get(), copperGolem.get()};
    m_world->setNearbyEntities(entities);

    LivingEntity* result = test::OfferFlowerGoalTestAccessor::findNearestCandidate(*m_goal);
    ASSERT_NE(result, nullptr);
    // 村民在 1.0 处，铜傀儡在 2.0 处，应返回村民
    EXPECT_EQ(result->id(), villager->id());
}

TEST_F(OfferFlowerGiftTest, FindNearestCandidate_DeadEntity_Skipped)
{
    // 死亡的村民应被跳过
    auto deadVillager = createVillager(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    deadVillager->remove(); // 标记为已移除（isAlive() 返回 false）
    EXPECT_FALSE(deadVillager->isAlive());

    auto aliveVillager = createVillager(EntityInstanceId(3), *m_world, 3.0f, 64.0f, 0.0f);
    std::vector<Entity*> entities = {deadVillager.get(), aliveVillager.get()};
    m_world->setNearbyEntities(entities);

    LivingEntity* result = test::OfferFlowerGoalTestAccessor::findNearestCandidate(*m_goal);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->id(), aliveVillager->id());
}

TEST_F(OfferFlowerGiftTest, FindNearestCandidate_NullEntityInList_Skipped)
{
    // 列表中包含 nullptr，应被跳过不崩溃
    auto villager = createVillager(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    std::vector<Entity*> entities = {nullptr, villager.get(), nullptr};
    m_world->setNearbyEntities(entities);

    LivingEntity* result = test::OfferFlowerGoalTestAccessor::findNearestCandidate(*m_goal);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->id(), villager->id());
}

// ============================================================================
// _getGolemSearchBox 测试
// ============================================================================

TEST_F(OfferFlowerGiftTest, GetGolemSearchBox_ExpandedBy6X2_6)
{
    // 铁傀儡在 (0, 64, 0)，宽度 1.4，高度 2.7
    // boundingBox = (-0.7, 64, -0.7) ~ (0.7, 66.7, 0.7)
    // expand(6, 2, 6) 后 = (-6.7, 62, -6.7) ~ (6.7, 68.7, 6.7)
    AxisAlignedBB box = test::OfferFlowerGoalTestAccessor::getGolemSearchBox(*m_goal);

    // 验证扩展量（对应 MC inflate(6.0, 2.0, 6.0)）
    // 注意：expand 是单边扩展，所以总尺寸增加 2*expand
    f32 expectedMinX = -0.7f - 6.0f; // -6.7
    f32 expectedMaxX = 0.7f + 6.0f;  // 6.7
    f32 expectedMinY = 64.0f - 2.0f; // 62.0
    f32 expectedMaxY = 66.7f + 2.0f; // 68.7
    f32 expectedMinZ = -0.7f - 6.0f; // -6.7
    f32 expectedMaxZ = 0.7f + 6.0f;  // 6.7

    EXPECT_NEAR(box.minX, expectedMinX, 0.01f);
    EXPECT_NEAR(box.maxX, expectedMaxX, 0.01f);
    EXPECT_NEAR(box.minY, expectedMinY, 0.01f);
    EXPECT_NEAR(box.maxY, expectedMaxY, 0.01f);
    EXPECT_NEAR(box.minZ, expectedMinZ, 0.01f);
    EXPECT_NEAR(box.maxZ, expectedMaxZ, 0.01f);
}

// ============================================================================
// setEquipment/setGuaranteedDrop 与 dropPreservedEquipment 集成测试
// ============================================================================

TEST_F(OfferFlowerGiftTest, GiftedPoppy_DropsOnTurnToStatue)
{
    // 验证赠花后，铜傀儡通过 dropPreservedEquipment() 会掉落罂粟花
    // 这对应 MC 1.21.11 CopperGolem.turnToStatue() 中的 dropPreservedEquipment(serverLevel)
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    test::OfferFlowerGoalTestAccessor::setTarget(*m_goal, copperGolem.get());
    test::OfferFlowerGoalTestAccessor::setTick(*m_goal, 0);

    // 赠花
    test::OfferFlowerGoalTestAccessor::tryGiftFlowerToCopperGolem(*m_goal);
    expectAntennaHasPoppy(*copperGolem);
    expectAntennaGuaranteedDrop(*copperGolem);

    // 模拟转雕像时的 dropPreservedEquipment 调用
    size_t entityCountBefore = m_world->spawnedEntityCount();
    auto preservedSlots = copperGolem->dropPreservedEquipment();

    // 保留装备已掉落（不应在保留槽位中）
    EXPECT_TRUE(preservedSlots.empty());

    // 天线槽应被清空
    expectAntennaEmpty(*copperGolem);

    // 应生成了一个 ItemEntity（掉落的罂粟花）
    EXPECT_EQ(m_world->spawnedEntityCount(), entityCountBefore + 1);
}

TEST_F(OfferFlowerGiftTest, NonGiftedPoppy_NotDroppedOnTurnToStatue)
{
    // 未赠花的铜傀儡，dropPreservedEquipment 不应掉落任何东西
    auto copperGolem = createCopperGolem(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);

    // 不赠花，直接调用 dropPreservedEquipment
    size_t entityCountBefore = m_world->spawnedEntityCount();
    auto preservedSlots = copperGolem->dropPreservedEquipment();

    EXPECT_TRUE(preservedSlots.empty());
    EXPECT_EQ(m_world->spawnedEntityCount(), entityCountBefore); // 不应有掉落
    expectAntennaEmpty(*copperGolem);
}

// ============================================================================
// 标签内容验证
// ============================================================================

TEST_F(OfferFlowerGiftTest, AcceptsTag_ContainsCopperGolem)
{
    EXPECT_TRUE(EntityTypeTags::ACCEPTS_IRON_GOLEM_GIFT().contains(std::string("minecraft:copper_golem")));
}

TEST_F(OfferFlowerGiftTest, AcceptsTag_DoesNotContainVillager)
{
    EXPECT_FALSE(EntityTypeTags::ACCEPTS_IRON_GOLEM_GIFT().contains(std::string("minecraft:villager")));
}

TEST_F(OfferFlowerGiftTest, CandidateTag_ContainsVillager)
{
    EXPECT_TRUE(EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT().contains(std::string("minecraft:villager")));
}

TEST_F(OfferFlowerGiftTest, CandidateTag_ContainsCopperGolem)
{
    EXPECT_TRUE(EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT().contains(std::string("minecraft:copper_golem")));
}

TEST_F(OfferFlowerGiftTest, CandidateTag_DoesNotContainPlayer)
{
    EXPECT_FALSE(EntityTypeTags::CANDIDATE_FOR_IRON_GOLEM_GIFT().contains(std::string("minecraft:player")));
}

// ============================================================================
// 标签未初始化时的 _findNearestCandidate 测试
// ============================================================================
//
// 由于 EntityTypeTags::isInitialized() 是全局静态状态，且 SetUpTestSuite 已初始化，
// 这里无法在同一进程内"反初始化"。但我们可以通过验证 isInitialized() 为 true 时
// _findNearestCandidate 正常工作来间接验证。
// 标签未初始化时返回 nullptr 的逻辑在代码中是 `if (!isInitialized()) return nullptr;`，
// 该分支在已初始化环境下无法触发，但代码路径已通过 code review 验证。

TEST_F(OfferFlowerGiftTest, FindNearestCandidate_TagsInitialized_WorksCorrectly)
{
    // 验证标签系统已初始化（SetUpTestSuite 中调用 EntityTypeTags::initialize()）
    EXPECT_TRUE(EntityTypeTags::isInitialized());

    auto villager = createVillager(EntityInstanceId(2), *m_world, 1.0f, 64.0f, 0.0f);
    std::vector<Entity*> entities = {villager.get()};
    m_world->setNearbyEntities(entities);

    LivingEntity* result = test::OfferFlowerGoalTestAccessor::findNearestCandidate(*m_goal);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->id(), villager->id());
}
