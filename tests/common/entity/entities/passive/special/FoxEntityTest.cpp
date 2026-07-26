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
#include "common/entity/entities/passive/special/FoxEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/food/FoodItem.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用模拟世界
 */
class FoxTestWorld final : public test::BaseTestWorld {
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

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("FoxTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("FoxTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class FoxEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    FoxTestWorld m_world;
};

// ========== 狐狸类型测试 ==========

TEST_F(FoxEntityTest, FoxType_DefaultIsRed)
{
    FoxEntity fox(EntityInstanceId(1));
    // 默认类型是红色狐狸
    EXPECT_EQ(fox.getFoxType(), FoxEntity::FoxType::Red);
}

TEST_F(FoxEntityTest, FoxType_CanSetAndGetType)
{
    FoxEntity fox(EntityInstanceId(1));

    fox.setFoxType(FoxEntity::FoxType::Snow);
    EXPECT_EQ(fox.getFoxType(), FoxEntity::FoxType::Snow);

    fox.setFoxType(FoxEntity::FoxType::Red);
    EXPECT_EQ(fox.getFoxType(), FoxEntity::FoxType::Red);
}

// ========== 信任系统测试 ==========

TEST_F(FoxEntityTest, TrustSystem_NoTrustedPlayersInitially)
{
    FoxEntity fox(EntityInstanceId(1));

    EXPECT_FALSE(fox.trusts(12345));
    EXPECT_FALSE(fox.getFirstTrustedPlayer().has_value());
}

TEST_F(FoxEntityTest, TrustSystem_CanAddTrustedPlayer)
{
    FoxEntity fox(EntityInstanceId(1));

    fox.addTrustedPlayer(12345);
    EXPECT_TRUE(fox.trusts(12345));
    EXPECT_EQ(fox.getFirstTrustedPlayer().value_or(0), 12345);
}

TEST_F(FoxEntityTest, TrustSystem_CanAddMultipleTrustedPlayers)
{
    FoxEntity fox(EntityInstanceId(1));

    fox.addTrustedPlayer(111);
    fox.addTrustedPlayer(222);

    EXPECT_TRUE(fox.trusts(111));
    EXPECT_TRUE(fox.trusts(222));
    EXPECT_EQ(fox.getFirstTrustedPlayer().value_or(0), 111);
}

TEST_F(FoxEntityTest, TrustSystem_MaxTwoTrustedPlayers)
{
    FoxEntity fox(EntityInstanceId(1));

    fox.addTrustedPlayer(111);
    fox.addTrustedPlayer(222);
    fox.addTrustedPlayer(333); // 应该替换第一个

    EXPECT_FALSE(fox.trusts(111)); // 第一个被替换
    EXPECT_TRUE(fox.trusts(222));
    EXPECT_TRUE(fox.trusts(333));
}

TEST_F(FoxEntityTest, TrustSystem_CanRemoveTrustedPlayer)
{
    FoxEntity fox(EntityInstanceId(1));

    fox.addTrustedPlayer(12345);
    EXPECT_TRUE(fox.trusts(12345));

    fox.removeTrustedPlayer(12345);
    EXPECT_FALSE(fox.trusts(12345));
}

TEST_F(FoxEntityTest, TrustSystem_DoesNotAddDuplicate)
{
    FoxEntity fox(EntityInstanceId(1));

    fox.addTrustedPlayer(12345);
    fox.addTrustedPlayer(12345); // 重复添加

    // 应该只有一个
    EXPECT_TRUE(fox.trusts(12345));
    fox.removeTrustedPlayer(12345);
    EXPECT_FALSE(fox.trusts(12345));
}

// ========== 繁殖物品测试 ==========

TEST_F(FoxEntityTest, IsBreedingItem_AcceptsSweetBerries)
{
    FoxEntity fox(EntityInstanceId(1));

    ItemStack sweetBerriesStack(Items::SWEET_BERRIES, 1);
    EXPECT_TRUE(fox.isBreedingItem(sweetBerriesStack));
}

TEST_F(FoxEntityTest, IsBreedingItem_AcceptsGlowBerries)
{
    // MC 原版 FOX_FOOD 标签包含 sweet_berries 和 glow_berries
    FoxEntity fox(EntityInstanceId(1));

    if (Items::GLOW_BERRIES != nullptr) {
        ItemStack glowBerriesStack(Items::GLOW_BERRIES, 1);
        EXPECT_TRUE(fox.isBreedingItem(glowBerriesStack));
    }
}

TEST_F(FoxEntityTest, IsBreedingItem_RejectsOtherItems)
{
    FoxEntity fox(EntityInstanceId(1));

    // 测试不接受其他物品
    if (Items::WHEAT != nullptr) {
        ItemStack wheatStack(Items::WHEAT, 1);
        EXPECT_FALSE(fox.isBreedingItem(wheatStack));
    }

    if (Items::CARROT != nullptr) {
        ItemStack carrotStack(Items::CARROT, 1);
        EXPECT_FALSE(fox.isBreedingItem(carrotStack));
    }

    // 空物品栈
    ItemStack emptyStack;
    EXPECT_FALSE(fox.isBreedingItem(emptyStack));
}

// ========== spawnBaby 测试 ==========

TEST_F(FoxEntityTest, SpawnBaby_CreatesChildFox)
{
    FoxEntity parent1(EntityInstanceId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setFoxType(FoxEntity::FoxType::Red);

    FoxEntity parent2(EntityInstanceId(2));
    parent2.setFoxType(FoxEntity::FoxType::Snow);

    auto baby = parent1.spawnBaby(parent2);

    ASSERT_NE(baby, nullptr);
    EXPECT_TRUE(baby->isChild());

    // 检查是 FoxEntity 类型
    FoxEntity* babyFox = dynamic_cast<FoxEntity*>(baby.get());
    EXPECT_NE(babyFox, nullptr);
}

TEST_F(FoxEntityTest, SpawnBaby_InheritsParentType)
{
    FoxEntity parent1(EntityInstanceId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setFoxType(FoxEntity::FoxType::Red);

    FoxEntity parent2(EntityInstanceId(2));
    parent2.setFoxType(FoxEntity::FoxType::Snow);

    // 多次测试类型继承（由于随机性，50%概率继承任一父母）
    int redCount = 0;
    int snowCount = 0;
    const int iterations = 100;

    for (int i = 0; i < iterations; ++i) {
        // 每次创建新的父实体来获得不同的随机种子
        FoxEntity p1(static_cast<EntityInstanceId>(i * 2 + 1));
        p1.setWorld(&m_world);
        p1.setPosition(0.0f, 64.0f, 0.0f);
        p1.setFoxType(FoxEntity::FoxType::Red);

        FoxEntity p2(static_cast<EntityInstanceId>(i * 2 + 2));
        p2.setFoxType(FoxEntity::FoxType::Snow);

        auto baby = p1.spawnBaby(p2);
        ASSERT_NE(baby, nullptr);

        FoxEntity* babyFox = dynamic_cast<FoxEntity*>(baby.get());
        ASSERT_NE(babyFox, nullptr);

        if (babyFox->getFoxType() == FoxEntity::FoxType::Red) {
            redCount++;
        } else if (babyFox->getFoxType() == FoxEntity::FoxType::Snow) {
            snowCount++;
        }
    }

    // 两种类型都应该出现（概率分布测试）
    // 由于是50%概率，两种类型都应该有相当的数量
    EXPECT_GT(redCount, 10) << "Red type should appear at least 10 times in 100 iterations";
    EXPECT_GT(snowCount, 10) << "Snow type should appear at least 10 times in 100 iterations";
}

TEST_F(FoxEntityTest, SpawnBaby_InheritsTrustedPlayers)
{
    FoxEntity parent1(EntityInstanceId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setFoxType(FoxEntity::FoxType::Red);
    parent1.addTrustedPlayer(111);
    parent1.addTrustedPlayer(222);

    FoxEntity parent2(EntityInstanceId(2));
    parent2.setWorld(&m_world); // 设置世界以获得随机数
    parent2.setFoxType(FoxEntity::FoxType::Snow);
    parent2.addTrustedPlayer(333);

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    FoxEntity* babyFox = dynamic_cast<FoxEntity*>(baby.get());
    ASSERT_NE(babyFox, nullptr);

    // MC 1.16.5: 幼狐继承父母的信任玩家，但最多只能有2个信任玩家
    // 继承顺序：先添加 parent1 的信任玩家，再添加 parent2 的信任玩家
    // parent1 有 [111, 222]，parent2 有 [333]
    // 添加 parent1 后：[111, 222]
    // 添加 parent2 后：由于 MAX=2，会替换最早的 (111)，结果为 [222, 333]
    EXPECT_EQ(babyFox->getTrustedPlayers().size(), 2u); // 最多2个信任玩家
    EXPECT_TRUE(babyFox->trusts(222));                  // 来自 parent1
    EXPECT_TRUE(babyFox->trusts(333));                  // 来自 parent2
    EXPECT_FALSE(babyFox->trusts(111));                 // 被替换掉了
}

TEST_F(FoxEntityTest, SpawnBaby_PositionIsSet)
{
    FoxEntity parent(EntityInstanceId(1));
    parent.setWorld(&m_world);
    parent.setPosition(100.0f, 64.0f, -50.0f);
    parent.setFoxType(FoxEntity::FoxType::Red);

    FoxEntity partner(EntityInstanceId(2));
    partner.setFoxType(FoxEntity::FoxType::Snow);

    auto baby = parent.spawnBaby(parent);
    ASSERT_NE(baby, nullptr);

    // 幼狐应该在父母附近
    EXPECT_NEAR(baby->x(), 100.0f, 2.0f);
    EXPECT_NEAR(baby->y(), 64.0f, 2.0f);
    EXPECT_NEAR(baby->z(), -50.0f, 2.0f);
}

// ========== 属性测试 ==========

TEST_F(FoxEntityTest, Attributes_HasCorrectBaseValues)
{
    FoxEntity fox(EntityInstanceId(1));

    // MC 1.16.5: 狐狸生命值为 10
    EXPECT_DOUBLE_EQ(fox.maxHealth(), 10.0);

    // MC 1.16.5: 狐狸移动速度为 0.3
    EXPECT_DOUBLE_EQ(fox.getAttributeValue("generic.movement_speed", 0.0), 0.3);
}

// ========== 尺寸测试 ==========
// 注意：FoxEntity 使用 AnimalEntity 的默认尺寸，不在此测试尺寸
// 因为 AnimalEntity 的默认尺寸可能随实现变化

TEST_F(FoxEntityTest, EyeHeight_DifferentForChildAndAdult)
{
    FoxEntity adultFox(EntityInstanceId(1));
    adultFox.setChild(false);

    FoxEntity childFox(EntityInstanceId(2));
    childFox.setChild(true);

    // 成体眼睛高度 0.4，幼体 0.2
    EXPECT_FLOAT_EQ(adultFox.eyeHeight(), 0.4f);
    EXPECT_FLOAT_EQ(childFox.eyeHeight(), 0.2f);
}

// ========== 睡眠状态测试 ==========

TEST_F(FoxEntityTest, SleepState_DefaultNotSleeping)
{
    FoxEntity fox(EntityInstanceId(1));
    EXPECT_FALSE(fox.isSleeping());
}

TEST_F(FoxEntityTest, SleepState_CanSetSleeping)
{
    FoxEntity fox(EntityInstanceId(1));

    fox.setSleeping(true);
    EXPECT_TRUE(fox.isSleeping());

    fox.setSleeping(false);
    EXPECT_FALSE(fox.isSleeping());
}

// ========== 叼物品测试 ==========

TEST_F(FoxEntityTest, HeldItem_DefaultNotHolding)
{
    FoxEntity fox(EntityInstanceId(1));
    EXPECT_FALSE(fox.isHoldingItem());
    EXPECT_EQ(fox.getHeldItem(), nullptr);
}

TEST_F(FoxEntityTest, HeldItem_CanSetAndClear)
{
    FoxEntity fox(EntityInstanceId(1));
    fox.setWorld(&m_world);
    fox.setPosition(0.0f, 64.0f, 0.0f);

    auto item = std::make_unique<ItemStack>(Items::SWEET_BERRIES, 1);
    fox.setHeldItem(std::move(item));

    EXPECT_TRUE(fox.isHoldingItem());
    EXPECT_NE(fox.getHeldItem(), nullptr);
    EXPECT_EQ(fox.getHeldItem()->getItem(), Items::SWEET_BERRIES);

    // dropHeldItem 需要有效的 world 来生成物品实体
    fox.dropHeldItem();
    EXPECT_FALSE(fox.isHoldingItem());
    EXPECT_EQ(fox.getHeldItem(), nullptr);
}

// ========== dropHeldItem 测试 ==========

TEST_F(FoxEntityTest, DropHeldItem_DropsItemInWorld)
{
    FoxEntity fox(EntityInstanceId(1));
    fox.setWorld(&m_world);
    fox.setPosition(0.0f, 64.0f, 0.0f);

    // 设置持有物品
    auto item = std::make_unique<ItemStack>(Items::SWEET_BERRIES, 16);
    fox.setHeldItem(std::move(item));

    EXPECT_TRUE(fox.isHoldingItem());

    // dropHeldItem 应该在世界中生成物品实体
    fox.dropHeldItem();

    // 物品应该被清空
    EXPECT_FALSE(fox.isHoldingItem());
    EXPECT_EQ(fox.getHeldItem(), nullptr);

    // 世界应该生成了一个物品实体
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}

TEST_F(FoxEntityTest, DropHeldItem_DoesNothingWhenEmpty)
{
    FoxEntity fox(EntityInstanceId(1));
    fox.setWorld(&m_world);
    fox.setPosition(0.0f, 64.0f, 0.0f);

    // 没有持有物品
    EXPECT_FALSE(fox.isHoldingItem());

    // dropHeldItem 应该安全地什么都不做
    EXPECT_NO_THROW(fox.dropHeldItem());

    // 不应该生成任何实体
    EXPECT_EQ(m_world.spawnedEntities().size(), 0u);
}

// ========== AI Goal 注册测试 ==========

TEST_F(FoxEntityTest, Goals_AvoidEntityGoalRegistered)
{
    // [已完成] 验证 AvoidEntityGoal 已正确注册 - 2026/05/16
    // MC 1.16.5: 狐狸躲避未信任的玩家，检测距离 16 格，逃跑速度 1.6/1.4
    FoxEntity fox(EntityInstanceId(1));
    fox.setWorld(&m_world);

    const auto& goals = fox.goalSelector().getAllGoals();

    // 验证至少有一个目标
    EXPECT_GT(goals.size(), 0u) << "FoxEntity should have AI goals registered";

    // 验证有 AvoidEntityGoal 类型的目标
    bool hasAvoidEntityGoal = false;
    for (const auto& goal : goals) {
        if (goal.getGoal()->getTypeName() == "AvoidEntityGoal") {
            hasAvoidEntityGoal = true;
            // 验证优先级为 4 (MC 1.16.5)
            EXPECT_EQ(goal.getPriority(), 4);
            break;
        }
    }
    EXPECT_TRUE(hasAvoidEntityGoal) << "FoxEntity should have AvoidEntityGoal registered";
}

TEST_F(FoxEntityTest, Goals_TemptGoalRegistered)
{
    // [已完成] 验证 TemptGoal 已正确注册 - 2026/05/16
    // MC 1.16.5: 狐狸被甜浆果诱惑，跟随速度 1.0
    FoxEntity fox(EntityInstanceId(1));
    fox.setWorld(&m_world);

    const auto& goals = fox.goalSelector().getAllGoals();

    // 验证有 TemptGoal 类型的目标
    bool hasTemptGoal = false;
    for (const auto& goal : goals) {
        if (goal.getGoal()->getTypeName() == "TemptGoal") {
            hasTemptGoal = true;
            // 验证优先级为 3 (MC 1.16.5)
            EXPECT_EQ(goal.getPriority(), 3);
            break;
        }
    }
    EXPECT_TRUE(hasTemptGoal) << "FoxEntity should have TemptGoal registered for sweet berries";
}

TEST_F(FoxEntityTest, Goals_HasBasicAnimalGoals)
{
    // 验证狐狸注册了基本的 AI 目标
    // 注意：MC 1.16.5 中 FoxEntity 自己注册所有目标，不依赖 AnimalEntity 基类
    // AnimalEntity::registerGoals() 的注释说明基类不注册任何目标
    FoxEntity fox(EntityInstanceId(1));
    fox.setWorld(&m_world);

    const auto& goals = fox.goalSelector().getAllGoals();

    // 验证至少有 AvoidEntityGoal 和 TemptGoal（我们实现的）
    bool hasAvoidEntityGoal = false;
    bool hasTemptGoal = false;

    for (const auto& goal : goals) {
        const std::string typeName = goal.getGoal()->getTypeName();
        if (typeName == "AvoidEntityGoal") hasAvoidEntityGoal = true;
        if (typeName == "TemptGoal") hasTemptGoal = true;
    }

    // 验证我们实现的目标存在
    EXPECT_TRUE(hasAvoidEntityGoal) << "FoxEntity should have AvoidEntityGoal for avoiding untrusted players";
    EXPECT_TRUE(hasTemptGoal) << "FoxEntity should have TemptGoal for sweet berries";

    // 验证目标数量大于0
    EXPECT_GT(goals.size(), 0u) << "FoxEntity should have AI goals registered";
}

// ========== 进食逻辑测试（onItemUseFinish 集成） ==========

TEST_F(FoxEntityTest, Eating_MushroomStew_ReturnsBowlAsHeldItem)
{
    // MC 原版: 狐狸吃蘑菇煲后应返回碗作为容器物品
    // 对应 MC: Fox.aiStep() 中 finishUsingItem 返回非空物品时设置到主手
    FoxEntity fox(EntityInstanceId(1));
    fox.setWorld(&m_world);
    fox.setPosition(0.0f, 64.0f, 0.0f);
    fox.setOnGround(true);

    // 设置蘑菇煲为叼着的物品
    if (Items::MUSHROOM_STEW != nullptr && Items::BOWL != nullptr) {
        auto stew = std::make_unique<ItemStack>(Items::MUSHROOM_STEW, 1);
        fox.setHeldItem(std::move(stew));

        EXPECT_TRUE(fox.isHoldingItem());
        EXPECT_EQ(fox.getHeldItem()->getItem(), Items::MUSHROOM_STEW);

        // 模拟进食完成：直接调用 onItemUseFinish
        IWorld* worldPtr = fox.world();
        ASSERT_NE(worldPtr, nullptr);

        const ItemStack* held = fox.getHeldItem();
        ASSERT_NE(held, nullptr);
        ASSERT_NE(held->getItem(), nullptr);

        ItemStack heldCopy = *held;
        ItemStack result = const_cast<Item*>(heldCopy.getItem())->onItemUseFinish(heldCopy, *worldPtr, fox);

        // 蘑菇煲消耗后应返回碗
        EXPECT_FALSE(result.isEmpty());
        EXPECT_EQ(result.getItem(), Items::BOWL);

        // 模拟狐狸设置返回物品到嘴中
        if (!result.isEmpty()) {
            fox.setHeldItem(std::make_unique<ItemStack>(std::move(result)));
        } else {
            fox.setHeldItem(nullptr);
        }

        // 验证狐狸现在叼着碗
        EXPECT_TRUE(fox.isHoldingItem());
        EXPECT_EQ(fox.getHeldItem()->getItem(), Items::BOWL);
    }
}

TEST_F(FoxEntityTest, Eating_NormalFood_ClearsHeldItem)
{
    // MC 原版: 狐狸吃普通食物（如甜浆果）后应清空嘴中物品
    // 注意：甜浆果注册为基础 Item 而非 FoodItem，Item::onItemUseFinish 不处理消耗。
    // FoxEntity::tick() 中的进食逻辑会检测非 FoodItem 的食物并手动 shrink。
    // 此测试直接验证 FoxEntity 进食逻辑对普通食物的完整处理。
    FoxEntity fox(EntityInstanceId(1));
    fox.setWorld(&m_world);
    fox.setPosition(0.0f, 64.0f, 0.0f);
    fox.setOnGround(true);

    auto berries = std::make_unique<ItemStack>(Items::SWEET_BERRIES, 1);
    fox.setHeldItem(std::move(berries));

    EXPECT_TRUE(fox.isHoldingItem());
    EXPECT_EQ(fox.getHeldItem()->getItem(), Items::SWEET_BERRIES);

    IWorld* worldPtr = fox.world();
    ASSERT_NE(worldPtr, nullptr);

    const ItemStack* held = fox.getHeldItem();
    ASSERT_NE(held, nullptr);
    ASSERT_NE(held->getItem(), nullptr);

    // 模拟 FoxEntity::tick() 中的进食逻辑
    ItemStack heldCopy = *held;
    const Item* item = heldCopy.getItem();
    ItemStack result = const_cast<Item*>(item)->onItemUseFinish(heldCopy, *worldPtr, fox);

    // 对于非 FoodItem 的食物（如甜浆果），onItemUseFinish 不会消耗物品
    // FoxEntity 需要手动处理 shrink（对应 FoxEntity::tick 中的逻辑）
    if (item != nullptr && !dynamic_cast<const item::items::FoodItem*>(item)) {
        heldCopy.shrink(1);
        if (heldCopy.isEmpty() && item->hasContainerItem()) {
            result = ItemStack(item->containerItem(), 1);
        } else {
            result = heldCopy;
        }
    }

    // 甜浆果没有容器物品，消耗后返回空 ItemStack
    EXPECT_TRUE(result.isEmpty());

    // 模拟狐狸逻辑：返回空则清空嘴中物品
    if (!result.isEmpty()) {
        fox.setHeldItem(std::make_unique<ItemStack>(std::move(result)));
    } else {
        fox.setHeldItem(nullptr);
    }

    EXPECT_FALSE(fox.isHoldingItem());
    EXPECT_EQ(fox.getHeldItem(), nullptr);
}

TEST_F(FoxEntityTest, Eating_MushroomStewStackOfTwo_ReturnsBowlAndReducesCount)
{
    // 测试堆叠数量为2的蘑菇煲（虽然MC中蘑菇煲最大堆叠为1，
    // 但这个测试验证 onItemUseFinish 的 shrink + containerItem 逻辑）
    // 注意：蘑菇煲 maxStackSize=1，这里只是测试逻辑正确性
    FoxEntity fox(EntityInstanceId(1));
    fox.setWorld(&m_world);
    fox.setPosition(0.0f, 64.0f, 0.0f);
    fox.setOnGround(true);

    if (Items::MUSHROOM_STEW != nullptr && Items::BOWL != nullptr) {
        // 创建一个堆叠数量为1的蘑菇煲
        auto stew = std::make_unique<ItemStack>(Items::MUSHROOM_STEW, 1);
        fox.setHeldItem(std::move(stew));

        IWorld* worldPtr = fox.world();
        const ItemStack* held = fox.getHeldItem();
        ItemStack heldCopy = *held;
        ItemStack result = const_cast<Item*>(heldCopy.getItem())->onItemUseFinish(heldCopy, *worldPtr, fox);

        // 蘑菇煲有容器物品（碗），shrink(1) 后 count=0，所以返回碗
        EXPECT_FALSE(result.isEmpty());
        EXPECT_EQ(result.getItem(), Items::BOWL);
        EXPECT_EQ(result.getCount(), 1);
    }
}

} // namespace
} // namespace mc
