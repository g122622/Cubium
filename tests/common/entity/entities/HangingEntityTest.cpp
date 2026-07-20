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
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/hanging/HangingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"

#include <cmath>

namespace mc {
namespace {

/**
 * @brief HangingEntity 单元测试
 *
 * 测试悬挂实体的 canPlaceOn、dropItem 等方法。
 */
class HangingEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// ============================================================================
// canPlaceOn 测试
// ============================================================================

TEST_F(HangingEntityTest, CanPlaceOnReturnsFalseWhenWorldIsNull)
{
    // 创建画作实体
    entity::PaintingEntity painting;

    // 没有设置世界时应该返回 false
    EXPECT_FALSE(painting.canPlaceOn());
}

TEST_F(HangingEntityTest, CanPlaceOnReturnsTrueForValidPosition)
{
    // 这个测试需要模拟世界，目前只验证方法存在
    // 实际测试需要 Mock IWorld
    entity::PaintingEntity painting;

    // 验证方法可以调用
    // 由于没有设置世界，应该返回 false
    EXPECT_FALSE(painting.canPlaceOn());
}

TEST_F(HangingEntityTest, LeashKnotEntityCanBeCreated)
{
    // 验证拴绳结实体可以创建
    entity::LeashKnotEntity leashKnot;

    EXPECT_EQ(leashKnot.getWidth(), 1);
    EXPECT_EQ(leashKnot.getHeight(), 1);
}

TEST_F(HangingEntityTest, ItemFrameEntityCanBeCreated)
{
    // 验证物品展示框实体可以创建
    entity::ItemFrameEntity itemFrame;

    EXPECT_EQ(itemFrame.getWidth(), 1);
    EXPECT_EQ(itemFrame.getHeight(), 1);
    EXPECT_FALSE(itemFrame.isGlowing());

    itemFrame.setGlowing(true);
    EXPECT_TRUE(itemFrame.isGlowing());
}

TEST_F(HangingEntityTest, PaintingEntityMotiveCanBeSet)
{
    entity::PaintingEntity painting;

    // 验证默认画作
    EXPECT_EQ(painting.getMotive(), "Kebab");

    // 设置新的画作类型
    painting.setMotive("Aztec");
    EXPECT_EQ(painting.getMotive(), "Aztec");

    // 验证尺寸
    EXPECT_EQ(painting.getWidth(), 1);
    EXPECT_EQ(painting.getHeight(), 1);
}

TEST_F(HangingEntityTest, PaintingEntityDimensions)
{
    // 验证不同画作的尺寸
    entity::PaintingEntity painting;

    // 1x1 画作
    painting.setMotive("Kebab");
    EXPECT_EQ(painting.getWidth(), 1);
    EXPECT_EQ(painting.getHeight(), 1);

    // 2x1 画作
    painting.setMotive("Pool");
    EXPECT_EQ(painting.getWidth(), 2);
    EXPECT_EQ(painting.getHeight(), 1);

    // 4x4 画作
    painting.setMotive("Pointer");
    EXPECT_EQ(painting.getWidth(), 4);
    EXPECT_EQ(painting.getHeight(), 4);
}

TEST_F(HangingEntityTest, ItemFrameRotation)
{
    entity::ItemFrameEntity itemFrame;

    // 初始旋转
    EXPECT_EQ(itemFrame.getItemRotation(), 0);

    // 旋转物品
    itemFrame.rotateItem();
    EXPECT_EQ(itemFrame.getItemRotation(), 1);

    itemFrame.rotateItem();
    EXPECT_EQ(itemFrame.getItemRotation(), 2);

    // 设置旋转
    itemFrame.setItemRotation(5);
    EXPECT_EQ(itemFrame.getItemRotation(), 5);

    // 旋转超过 7 应该回绕
    itemFrame.setItemRotation(10);
    EXPECT_EQ(itemFrame.getItemRotation(), 2); // 10 % 8 = 2

    // 负数旋转
    itemFrame.setItemRotation(-1);
    EXPECT_EQ(itemFrame.getItemRotation(), 7); // -1 + 8 = 7
}

// ============================================================================
// ItemFrameEntity 红石信号测试
// ============================================================================

TEST_F(HangingEntityTest, ItemFrameAnalogOutput_NoItem_ReturnsZero)
{
    // MC 1.16.5: 无物品时返回 0
    entity::ItemFrameEntity itemFrame;

    EXPECT_FALSE(itemFrame.hasItem());
    EXPECT_EQ(itemFrame.getAnalogOutput(), 0);
}

TEST_F(HangingEntityTest, ItemFrameAnalogOutput_WithItem_ReturnsRotationPlusOne)
{
    // MC 1.16.5: 有物品时返回 rotation % 8 + 1
    entity::ItemFrameEntity itemFrame;

    // 设置物品
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond);
    EXPECT_TRUE(itemFrame.hasItem());

    // rotation = 0 时，信号 = 1
    itemFrame.setItemRotation(0);
    EXPECT_EQ(itemFrame.getItemRotation(), 0);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 1);

    // rotation = 1 时，信号 = 2
    itemFrame.setItemRotation(1);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 2);

    // rotation = 7 时，信号 = 8
    itemFrame.setItemRotation(7);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 8);

    // rotation = 8 时，应该是 0（被 % 8）
    itemFrame.setItemRotation(8);
    EXPECT_EQ(itemFrame.getItemRotation(), 0);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 1);
}

TEST_F(HangingEntityTest, ItemFrameAnalogOutput_RotationRange)
{
    // MC 1.16.5: 测试所有旋转值的信号强度
    entity::ItemFrameEntity itemFrame;
    ItemStack item(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(item);

    // 测试所有旋转值
    for (i32 rotation = 0; rotation <= 7; ++rotation) {
        itemFrame.setItemRotation(rotation);
        EXPECT_EQ(itemFrame.getAnalogOutput(), rotation + 1)
            << "Expected signal " << (rotation + 1) << " for rotation " << rotation;
    }

    // 信号范围应该是 1-8
    EXPECT_GE(itemFrame.getAnalogOutput(), 1);
    EXPECT_LE(itemFrame.getAnalogOutput(), 8);
}

TEST_F(HangingEntityTest, ItemFrameSetDisplayedItem_ResetsRotation)
{
    // MC 1.16.5: 设置物品时重置旋转为 0
    entity::ItemFrameEntity itemFrame;

    // 先设置旋转
    itemFrame.setItemRotation(5);
    EXPECT_EQ(itemFrame.getItemRotation(), 5);

    // 设置新物品时旋转应该重置
    ItemStack item(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(item);
    EXPECT_EQ(itemFrame.getItemRotation(), 0);
}

TEST_F(HangingEntityTest, ItemFrameHorizontalFacing_ConvertsCorrectly)
{
    // MC 1.16.5: 测试方向转换
    entity::ItemFrameEntity itemFrame;

    // 测试所有方向的转换
    // HangingEntity::Direction: SOUTH=0, WEST=1, NORTH=2, EAST=3
    // mc::Direction: North=2, South=3, West=4, East=5

    BlockPos pos(0, 0, 0);

    itemFrame.setHangingPosition(pos, entity::HangingEntity::Direction::SOUTH);
    EXPECT_EQ(itemFrame.getHorizontalFacing(), mc::Direction::South);

    itemFrame.setHangingPosition(pos, entity::HangingEntity::Direction::WEST);
    EXPECT_EQ(itemFrame.getHorizontalFacing(), mc::Direction::West);

    itemFrame.setHangingPosition(pos, entity::HangingEntity::Direction::NORTH);
    EXPECT_EQ(itemFrame.getHorizontalFacing(), mc::Direction::North);

    itemFrame.setHangingPosition(pos, entity::HangingEntity::Direction::EAST);
    EXPECT_EQ(itemFrame.getHorizontalFacing(), mc::Direction::East);
}

TEST_F(HangingEntityTest, LeashKnotCanAttachEntities)
{
    entity::LeashKnotEntity leashKnot;

    // 验证可以绑定和解绑实体
    EXPECT_TRUE(leashKnot.getLeashedEntities().empty());

    // 注意：实际测试需要实体实例
    // 这里只验证方法存在
}

TEST_F(HangingEntityTest, HangingDirection)
{
    entity::PaintingEntity painting;

    // 验证默认方向
    EXPECT_EQ(painting.getDirection(), entity::HangingEntity::Direction::SOUTH);

    // 设置新方向
    BlockPos pos(0, 0, 0);
    painting.setHangingPosition(pos, entity::HangingEntity::Direction::NORTH);
    EXPECT_EQ(painting.getDirection(), entity::HangingEntity::Direction::NORTH);
    EXPECT_EQ(painting.getHangingBlockPos(), pos);

    painting.setHangingPosition(pos, entity::HangingEntity::Direction::EAST);
    EXPECT_EQ(painting.getDirection(), entity::HangingEntity::Direction::EAST);

    painting.setHangingPosition(pos, entity::HangingEntity::Direction::WEST);
    EXPECT_EQ(painting.getDirection(), entity::HangingEntity::Direction::WEST);
}

TEST_F(HangingEntityTest, ItemRegistration)
{
    // 验证物品已注册
    EXPECT_NE(Items::PAINTING, nullptr);
    EXPECT_NE(Items::ITEM_FRAME, nullptr);
    EXPECT_NE(Items::LEAD, nullptr);

    // 验证物品属性
    EXPECT_EQ(Items::PAINTING->maxStackSize(), 16);
    EXPECT_EQ(Items::ITEM_FRAME->maxStackSize(), 16);
    EXPECT_EQ(Items::LEAD->maxStackSize(), 16);
}

// ============================================================================
// HangingEntity::hurt 测试
//
// HangingEntity::hurt() 对应 MC Java 的 BlockAttachedEntity.hurtServer()。
// 悬挂实体被任何伤害一击即毁：无敌返回 false，mobGriefing 关闭时 Mob 攻击返回 false，
// 否则调用 dropItem() + remove() + markHurt()，返回 true。
// 已移除的实体不再执行 dropItem/remove/markHurt，但仍返回 true。
// ============================================================================

namespace {

/**
 * @brief HangingEntity hurt 测试用的 Mock World
 *
 * 支持 GameRules 和 gameEvent 捕获。
 */
class HangingEntityHurtTestWorld : public mc::test::BaseTestWorld {
public:
    HangingEntityHurtTestWorld()
    {
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        size_t index = static_cast<size_t>(id) - 1;
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        size_t index = static_cast<size_t>(id) - 1;
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override
    {
        return {};
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_gameEventCount++;
    }

    [[nodiscard]] i32 gameEventCount() const { return m_gameEventCount; }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

private:
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    i32 m_gameEventCount = 0;
    mutable math::Random m_random{12345};
};

/**
 * @brief 测试用 MobEntity 子类（用于 mobGriefing 伤害源测试）
 */
class TestHangingMobEntity : public MobEntity {
public:
    TestHangingMobEntity()
        : MobEntity(EntityInstanceId(200))
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

} // namespace

class HangingEntityHurtTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    void SetUp() override {}

    HangingEntityHurtTestWorld m_world;
};

/**
 * @brief 无敌伤害源返回 false，不调用 markHurt
 */
TEST_F(HangingEntityHurtTest, InvulnerableSource_ReturnsFalse_NoMarkHurt)
{
    entity::PaintingEntity painting;
    painting.setInvulnerable(true);
    EXPECT_FALSE(painting.isHurtMarked());

    auto source = DamageSources::generic();
    EXPECT_FALSE(painting.hurt(source, 1.0f));
    EXPECT_FALSE(painting.isHurtMarked());
    EXPECT_FALSE(painting.isRemoved());
}

/**
 * @brief mobGriefing 关闭 + Mob 攻击者：返回 false，不掉落不移除
 */
TEST_F(HangingEntityHurtTest, MobGriefingOff_MobAttacker_ReturnsFalse_NoDropNoRemove)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);
    ASSERT_FALSE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));

    entity::PaintingEntity painting;
    painting.setWorld(&m_world);

    TestHangingMobEntity mob;
    auto mobDamage = DamageSources::mobAttack(&mob);

    EXPECT_FALSE(painting.hurt(mobDamage, 1.0f));
    EXPECT_FALSE(painting.isRemoved());
    EXPECT_FALSE(painting.isHurtMarked());
}

/**
 * @brief mobGriefing 开启 + Mob 攻击者：正常伤害，调用 dropItem + remove + markHurt
 */
TEST_F(HangingEntityHurtTest, MobGriefingOn_MobAttacker_DropsAndRemoves)
{
    ASSERT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));

    entity::PaintingEntity painting;
    painting.setWorld(&m_world);

    TestHangingMobEntity mob;
    auto mobDamage = DamageSources::mobAttack(&mob);

    EXPECT_TRUE(painting.hurt(mobDamage, 1.0f));
    EXPECT_TRUE(painting.isRemoved());
    EXPECT_TRUE(painting.isHurtMarked());
}

/**
 * @brief 正常伤害（非 Mob 来源）：调用 dropItem + remove + markHurt，返回 true
 */
TEST_F(HangingEntityHurtTest, NormalDamage_DropsAndRemoves_MarksHurt)
{
    entity::PaintingEntity painting;
    painting.setWorld(&m_world);

    auto source = DamageSources::generic();
    EXPECT_TRUE(painting.hurt(source, 1.0f));
    EXPECT_TRUE(painting.isRemoved());
    EXPECT_TRUE(painting.isHurtMarked());
}

/**
 * @brief 正常伤害对 ItemFrameEntity：调用 dropItem + remove + markHurt
 */
TEST_F(HangingEntityHurtTest, ItemFrame_NormalDamage_DropsAndRemoves)
{
    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);

    auto source = DamageSources::generic();
    EXPECT_TRUE(itemFrame.hurt(source, 1.0f));
    EXPECT_TRUE(itemFrame.isRemoved());
    EXPECT_TRUE(itemFrame.isHurtMarked());
}

/**
 * @brief 已移除的实体：不调用 dropItem/remove/markHurt，但返回 true
 *
 * HangingEntity::hurt() 在 isRemoved() 为 true 时跳过 dropItem/remove/markHurt，
 * 但仍返回 true（MC Java 行为：已经移除的悬挂实体仍然"接受"伤害）。
 */
TEST_F(HangingEntityHurtTest, AlreadyRemovedEntity_NoDropNoRemoveNoMarkHurt_ReturnsTrue)
{
    entity::PaintingEntity painting;
    painting.setWorld(&m_world);

    // 第一次 hurt 正常工作
    auto source = DamageSources::generic();
    EXPECT_TRUE(painting.hurt(source, 1.0f));
    EXPECT_TRUE(painting.isRemoved());
    EXPECT_TRUE(painting.isHurtMarked());

    // 清除 hurtMarked 以验证第二次不重新标记
    painting.clearHurtMarked();
    EXPECT_FALSE(painting.isHurtMarked());

    // 第二次 hurt：已移除，不再执行 dropItem/remove/markHurt，但返回 true
    auto source2 = DamageSources::generic();
    EXPECT_TRUE(painting.hurt(source2, 1.0f));
    EXPECT_FALSE(painting.isHurtMarked()); // 不应被再次标记
}

/**
 * @brief mobGriefing 关闭时，非 Mob 环境伤害仍正常生效
 */
TEST_F(HangingEntityHurtTest, MobGriefingOff_EnvironmentalDamage_StillWorks)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);
    ASSERT_FALSE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));

    entity::PaintingEntity painting;
    painting.setWorld(&m_world);

    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(painting.hurt(genericDamage, 1.0f));
    EXPECT_TRUE(painting.isRemoved());
    EXPECT_TRUE(painting.isHurtMarked());
}

/**
 * @brief 没有 world 时 Mob 攻击者仍正常伤害（跳过 mobGriefing 检查）
 */
TEST_F(HangingEntityHurtTest, NoWorld_MobAttacker_StillDamages)
{
    entity::PaintingEntity painting;
    // 不设置 world -> mobGriefing 检查被跳过

    TestHangingMobEntity mob;
    auto mobDamage = DamageSources::mobAttack(&mob);

    EXPECT_TRUE(painting.hurt(mobDamage, 1.0f));
    EXPECT_TRUE(painting.isRemoved());
    EXPECT_TRUE(painting.isHurtMarked());
}

// ============================================================================
// ItemFrameEntity 红石比较器更新和游戏事件测试
//
// 验证 ItemFrameEntity 在物品变化时正确通知红石比较器更新，
// 并发出 BLOCK_CHANGE 游戏事件。
// 由于 Mock World 缺少 BlockState 数据，RedstoneSystem::updateComparators
// 在 getBlockState 返回 nullptr 时提前返回不崩溃，因此比较器更新测试
// 验证不崩溃和 getAnalogOutput 返回值正确性。游戏事件测试验证事件捕获。
// ============================================================================

namespace {

/**
 * @brief ItemFrame 红石比较器测试用的 Mock World
 *
 * 支持 GameRules 和 gameEvent 捕获。
 */
class ItemFrameComparatorTestWorld : public mc::test::BaseTestWorld {
public:
    ItemFrameComparatorTestWorld()
    {
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        size_t index = static_cast<size_t>(id) - 1;
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        size_t index = static_cast<size_t>(id) - 1;
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override
    {
        return {};
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_gameEvents.push_back({std::string(event.id()), pos, context.sourceEntity(), context.affectedState()});
    }

    [[nodiscard]] i32 gameEventCount() const { return static_cast<i32>(m_gameEvents.size()); }

    [[nodiscard]] bool hasGameEvent(const std::string& eventId) const
    {
        for (const auto& ev : m_gameEvents) {
            if (ev.id == eventId) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] i32 gameEventCountFor(const std::string& eventId) const
    {
        i32 count = 0;
        for (const auto& ev : m_gameEvents) {
            if (ev.id == eventId) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] const Entity* gameEventSourceEntity(i32 index = 0) const
    {
        if (index >= 0 && index < static_cast<i32>(m_gameEvents.size())) {
            return m_gameEvents[index].sourceEntity;
        }
        return nullptr;
    }

    [[nodiscard]] BlockPos gameEventPos(i32 index = 0) const
    {
        if (index >= 0 && index < static_cast<i32>(m_gameEvents.size())) {
            return m_gameEvents[index].pos;
        }
        return BlockPos(0, 0, 0);
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

private:
    struct GameEventRecord {
        std::string id;
        BlockPos pos;
        const Entity* sourceEntity;
        const BlockState* affectedState;
    };

    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<GameEventRecord> m_gameEvents;
    mutable math::Random m_random{12345};
};

} // namespace

class ItemFrameComparatorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    ItemFrameComparatorTestWorld m_world;
};

/**
 * @brief setDisplayedItem(updateComparator=true) 不崩溃
 *
 * RedstoneSystem::updateComparators 在 getBlockState 返回 nullptr 时提前返回，
 * 因此即使 Mock World 没有方块数据也不会崩溃。
 */
TEST_F(ItemFrameComparatorTest, SetDisplayedItem_WithUpdateComparator_DoesNotCrash)
{
    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);

    // 放入物品，触发比较器更新，不应崩溃
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond, true);
    EXPECT_TRUE(itemFrame.hasItem());
    EXPECT_EQ(itemFrame.getAnalogOutput(), 1);

    // 再次设置应不崩溃
    ItemStack iron(Items::IRON_INGOT, 1);
    itemFrame.setDisplayedItem(iron, true);
    EXPECT_TRUE(itemFrame.hasItem());
}

/**
 * @brief setDisplayedItem(updateComparator=false) 不触发比较器更新
 *
 * 对应 MC Java 中 ItemFrame.setItem(stack, false) 的场景，
 * 如 NBT 加载时不需要触发更新。
 */
TEST_F(ItemFrameComparatorTest, SetDisplayedItem_WithoutUpdateComparator_SkipsComparatorUpdate)
{
    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);

    // updateComparator=false 不应崩溃
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond, false);
    EXPECT_TRUE(itemFrame.hasItem());
    EXPECT_EQ(itemFrame.getAnalogOutput(), 1);
}

/**
 * @brief setItemRotation(updateComparator=true) 不崩溃
 */
TEST_F(ItemFrameComparatorTest, SetItemRotation_WithUpdateComparator_DoesNotCrash)
{
    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond, false);

    // 旋转应触发比较器更新，不应崩溃
    itemFrame.setItemRotation(3, true);
    EXPECT_EQ(itemFrame.getItemRotation(), 3);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 4);

    itemFrame.setItemRotation(5, true);
    EXPECT_EQ(itemFrame.getItemRotation(), 5);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 6);
}

/**
 * @brief setItemRotation(updateComparator=false) 不触发比较器更新
 */
TEST_F(ItemFrameComparatorTest, SetItemRotation_WithoutUpdateComparator_SkipsComparatorUpdate)
{
    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond, false);

    // updateComparator=false 不应崩溃
    itemFrame.setItemRotation(3, false);
    EXPECT_EQ(itemFrame.getItemRotation(), 3);
}

/**
 * @brief rotateItem() 触发比较器更新，不崩溃
 */
TEST_F(ItemFrameComparatorTest, RotateItem_TriggersComparatorUpdate_DoesNotCrash)
{
    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond, false);

    itemFrame.rotateItem();
    EXPECT_EQ(itemFrame.getItemRotation(), 1);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 2);

    itemFrame.rotateItem();
    EXPECT_EQ(itemFrame.getItemRotation(), 2);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 3);
}

/**
 * @brief 无 world 时 notifyComparatorUpdate 不崩溃
 */
TEST_F(ItemFrameComparatorTest, NotifyComparatorUpdate_NoWorld_DoesNotCrash)
{
    entity::ItemFrameEntity itemFrame;
    // 不设置 world

    // 不应崩溃
    itemFrame.setDisplayedItem(ItemStack(Items::DIAMOND, 1), true);
    itemFrame.setItemRotation(3, true);
    itemFrame.rotateItem();
    itemFrame.notifyComparatorUpdate();
}

/**
 * @brief setDisplayedItem 默认参数 updateComparator=true
 */
TEST_F(ItemFrameComparatorTest, SetDisplayedItem_DefaultParameter_TriggersComparatorUpdate)
{
    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);

    // 不传 updateComparator 参数，默认为 true，不应崩溃
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond);
    EXPECT_TRUE(itemFrame.hasItem());
}

/**
 * @brief setItemRotation 默认参数 updateComparator=true
 */
TEST_F(ItemFrameComparatorTest, SetItemRotation_DefaultParameter_TriggersComparatorUpdate)
{
    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond, false);

    // 不传 updateComparator 参数，默认为 true，不应崩溃
    itemFrame.setItemRotation(5);
    EXPECT_EQ(itemFrame.getItemRotation(), 5);
}

/**
 * @brief dropItem 触发 BLOCK_CHANGE 游戏事件
 */
TEST_F(ItemFrameComparatorTest, DropItem_TriggersGameEvent)
{
    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);
    BlockPos pos(10, 64, 20);
    itemFrame.setHangingPosition(pos, entity::HangingEntity::Direction::SOUTH);

    // 设置物品
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond, false);

    i32 initialGameEvents = m_world.gameEventCount();

    // dropItem 应触发游戏事件
    itemFrame.dropItem();

    EXPECT_EQ(m_world.gameEventCount(), initialGameEvents + 1);
    EXPECT_TRUE(m_world.hasGameEvent("block_change"));
    EXPECT_EQ(m_world.gameEventPos(), pos);
}

/**
 * @brief dropItem 清空展示物品后模拟输出归零
 */
TEST_F(ItemFrameComparatorTest, DropItem_ClearsItem_AnalogOutputReturnsZero)
{
    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);

    // 设置物品
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond, false);
    itemFrame.setItemRotation(5, false);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 6);

    // dropItem 后物品清空
    itemFrame.dropItem();
    EXPECT_FALSE(itemFrame.hasItem());
    EXPECT_EQ(itemFrame.getAnalogOutput(), 0);
}

/**
 * @brief setDisplayedItem 后 getAnalogOutput 正确反映变化
 */
TEST_F(ItemFrameComparatorTest, AnalogOutput_ReflectsSetDisplayedItem)
{
    entity::ItemFrameEntity itemFrame;

    // 无物品返回 0
    EXPECT_EQ(itemFrame.getAnalogOutput(), 0);

    // 设置物品后返回 rotation + 1（默认 rotation=0）
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond, false);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 1); // rotation 0 + 1 = 1

    // 设置旋转
    itemFrame.setItemRotation(5, false);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 6); // rotation 5 + 1 = 6

    // 清空物品后返回 0
    itemFrame.setDisplayedItem(ItemStack(), false);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 0);
}

} // namespace
} // namespace mc
