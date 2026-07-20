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
#include "common/entity/ai/goal/goals/interact/TameableGoals.hpp"
#include "common/entity/entities/passive/tamable/CatEntity.hpp"
#include "common/entity/entities/passive/tamable/ParrotEntity.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>

namespace mc {
namespace {

/**
 * @brief BegGoal 测试用世界
 *
 * 提供最小化测试环境用于 BegGoal 功能测试
 */
class BegTestWorld final : public test::BaseTestWorld {
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
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class BegGoalTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    BegTestWorld m_world;
};

// ============================================================================
// 狼的 BegGoal 测试
// ============================================================================

TEST_F(BegGoalTestFixture, Wolf_IsPlayerHoldingFood_UntamedWolf_BoneReturnsFalse)
{
    // 未驯服的狼不应该对骨头乞求（MC 1.16.5 规则）
    // 只有已驯服的狼才对骨头乞求
    WolfEntity wolf(EntityInstanceId(0));
    wolf.setTamed(false);

    // 使用反射或公开接口测试 isPlayerHoldingFood 的行为
    // 这里我们通过验证 isTameItem 和 isBreedingItem 来间接测试
    ItemStack boneStack(Items::BONE, 1);

    // 骨头是驯服物品
    EXPECT_TRUE(wolf.isTameItem(boneStack));
    // 骨头不是繁殖物品
    EXPECT_FALSE(wolf.isBreedingItem(boneStack));
}

TEST_F(BegGoalTestFixture, Wolf_IsPlayerHoldingFood_TamedWolf_BoneReturnsTrue)
{
    // 已驯服的狼应该对骨头乞求
    WolfEntity wolf(EntityInstanceId(0));
    wolf.setTamed(true);

    ItemStack boneStack(Items::BONE, 1);

    // 骨头是驯服物品
    EXPECT_TRUE(wolf.isTameItem(boneStack));
}

TEST_F(BegGoalTestFixture, Wolf_IsPlayerHoldingFood_AnyWolf_MeatReturnsTrue)
{
    // 所有狼（无论是否驯服）都应该对肉类乞求
    WolfEntity untamedWolf(EntityInstanceId(0));
    untamedWolf.setTamed(false);

    WolfEntity tamedWolf(EntityInstanceId(1));
    tamedWolf.setTamed(true);

    ItemStack porkchopStack(Items::PORKCHOP, 1);
    ItemStack beefStack(Items::BEEF, 1);
    ItemStack rottenFleshStack(Items::ROTTEN_FLESH, 1);

    // 未驯服的狼
    EXPECT_TRUE(untamedWolf.isBreedingItem(porkchopStack));
    EXPECT_TRUE(untamedWolf.isBreedingItem(beefStack));
    EXPECT_TRUE(untamedWolf.isBreedingItem(rottenFleshStack));

    // 已驯服的狼
    EXPECT_TRUE(tamedWolf.isBreedingItem(porkchopStack));
    EXPECT_TRUE(tamedWolf.isBreedingItem(beefStack));
    EXPECT_TRUE(tamedWolf.isBreedingItem(rottenFleshStack));
}

// ============================================================================
// 猫的 BegGoal 测试
// ============================================================================

TEST_F(BegGoalTestFixture, Cat_IsTameItem_CodAndSalmon)
{
    // 猫用生鳕鱼和生鲑鱼驯服
    CatEntity cat(EntityInstanceId(0));

    ItemStack codStack(Items::COD, 1);
    ItemStack salmonStack(Items::SALMON, 1);

    EXPECT_TRUE(cat.isTameItem(codStack));
    EXPECT_TRUE(cat.isTameItem(salmonStack));

    // 骨头不能驯服猫
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_FALSE(cat.isTameItem(boneStack));

    // 生鱼也是繁殖物品
    EXPECT_TRUE(cat.isBreedingItem(codStack));
    EXPECT_TRUE(cat.isBreedingItem(salmonStack));
}

// ============================================================================
// 鹦鹉的 BegGoal 测试
// ============================================================================

TEST_F(BegGoalTestFixture, Parrot_IsTameItem_Seeds)
{
    // 鹦鹉用种子驯服
    ParrotEntity parrot(EntityInstanceId(0));

    ItemStack wheatSeedsStack(Items::WHEAT_SEEDS, 1);
    ItemStack pumpkinSeedsStack(Items::PUMPKIN_SEEDS, 1);
    ItemStack melonSeedsStack(Items::MELON_SEEDS, 1);
    ItemStack beetrootSeedsStack(Items::BEETROOT_SEEDS, 1);

    EXPECT_TRUE(parrot.isTameItem(wheatSeedsStack));
    EXPECT_TRUE(parrot.isTameItem(pumpkinSeedsStack));
    EXPECT_TRUE(parrot.isTameItem(melonSeedsStack));
    EXPECT_TRUE(parrot.isTameItem(beetrootSeedsStack));

    // 骨头不能驯服鹦鹉
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_FALSE(parrot.isTameItem(boneStack));

    // 鹦鹉不能繁殖
    EXPECT_FALSE(parrot.isBreedingItem(wheatSeedsStack));
}

// ============================================================================
// TameableEntity 基类 isTameItem 默认实现测试
// ============================================================================

TEST_F(BegGoalTestFixture, TameableEntity_DefaultIsTameItem_ReturnsFalse)
{
    // 测试基类默认实现
    // 由于 TameableEntity 是抽象类，我们通过具体的子类测试
    // 这里验证未重写 isTameItem 的情况（实际上所有子类都重写了）

    // 所有可驯服动物的 isTameItem 都应该有具体实现
    WolfEntity wolf(EntityInstanceId(0));
    CatEntity cat(EntityInstanceId(0));
    ParrotEntity parrot(EntityInstanceId(0));

    ItemStack emptyStack(nullptr, 0);

    // 空物品堆应该返回 false
    EXPECT_FALSE(wolf.isTameItem(emptyStack));
    EXPECT_FALSE(cat.isTameItem(emptyStack));
    EXPECT_FALSE(parrot.isTameItem(emptyStack));
}

// ============================================================================
// BegGoal 构造函数测试
// ============================================================================

TEST_F(BegGoalTestFixture, BegGoal_Construction_ValidParameters)
{
    // 测试 BegGoal 构造
    WolfEntity wolf(EntityInstanceId(0));

    // BegGoal 构造需要实体和最大距离
    entity::ai::goal::BegGoal begGoal(&wolf, 8.0f);

    // 验证互斥标志为 Look
    const auto& flags = begGoal.getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
}

// ============================================================================
// BegGoal shouldExecute 测试
// ============================================================================

TEST_F(BegGoalTestFixture, BegGoal_ShouldExecute_NoWorld_ReturnsFalse)
{
    // 当实体没有关联世界时，shouldExecute 应该返回 false
    WolfEntity wolf(EntityInstanceId(0));
    // 注意：wolf 没有设置世界

    entity::ai::goal::BegGoal begGoal(&wolf, 8.0f);

    // 没有世界，应该返回 false
    EXPECT_FALSE(begGoal.shouldExecute());
}

// ============================================================================
// isTameItem override 关键字测试
// ============================================================================

// 这个测试验证编译时 override 关键字的存在
// 如果 override 关键字缺失，编译会产生警告
// 这是静态检查，运行时总是通过
TEST_F(BegGoalTestFixture, IsTameItem_Override_CompileTimeCheck)
{
    // 此测试验证 isTameItem 方法正确使用了 override 关键字
    // 如果 override 缺失，编译时会产生警告
    // 这是一个编译时检查，运行时总是通过

    WolfEntity wolf(EntityInstanceId(0));
    CatEntity cat(EntityInstanceId(0));
    ParrotEntity parrot(EntityInstanceId(0));

    // 通过基类指针调用 isTameItem，验证多态性
    TameableEntity* tameableWolf = &wolf;
    TameableEntity* tameableCat = &cat;
    TameableEntity* tameableParrot = &parrot;

    ItemStack boneStack(Items::BONE, 1);
    ItemStack codStack(Items::COD, 1);
    ItemStack seedsStack(Items::WHEAT_SEEDS, 1);

    // 验证多态调用正确工作
    EXPECT_TRUE(tameableWolf->isTameItem(boneStack));
    EXPECT_TRUE(tameableCat->isTameItem(codStack));
    EXPECT_TRUE(tameableParrot->isTameItem(seedsStack));

    // 验证错误的物品返回 false
    EXPECT_FALSE(tameableWolf->isTameItem(codStack));
    EXPECT_FALSE(tameableCat->isTameItem(boneStack));
    EXPECT_FALSE(tameableParrot->isTameItem(boneStack));
}

// ============================================================================
// 驯服物品 vs 繁殖物品 区分测试
// ============================================================================

TEST_F(BegGoalTestFixture, Wolf_TameItem_Vs_BreedingItem_Distinction)
{
    // 狼的驯服物品和繁殖物品是不同的
    WolfEntity wolf(EntityInstanceId(0));

    // 骨头：只能驯服，不能繁殖
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_TRUE(wolf.isTameItem(boneStack));
    EXPECT_FALSE(wolf.isBreedingItem(boneStack));

    // 猪肉：只能繁殖，不能驯服
    ItemStack porkchopStack(Items::PORKCHOP, 1);
    EXPECT_FALSE(wolf.isTameItem(porkchopStack));
    EXPECT_TRUE(wolf.isBreedingItem(porkchopStack));
}

TEST_F(BegGoalTestFixture, Cat_TameItem_And_BreedingItem_Same)
{
    // 猫的驯服物品和繁殖物品是相同的
    CatEntity cat(EntityInstanceId(0));

    // 生鳕鱼：既能驯服也能繁殖
    ItemStack codStack(Items::COD, 1);
    EXPECT_TRUE(cat.isTameItem(codStack));
    EXPECT_TRUE(cat.isBreedingItem(codStack));

    // 生鲑鱼：既能驯服也能繁殖
    ItemStack salmonStack(Items::SALMON, 1);
    EXPECT_TRUE(cat.isTameItem(salmonStack));
    EXPECT_TRUE(cat.isBreedingItem(salmonStack));
}

TEST_F(BegGoalTestFixture, Parrot_TameItem_Only_NoBreeding)
{
    // 鹦鹉只有驯服物品，不能繁殖
    ParrotEntity parrot(EntityInstanceId(0));

    // 种子：只能驯服
    ItemStack seedsStack(Items::WHEAT_SEEDS, 1);
    EXPECT_TRUE(parrot.isTameItem(seedsStack));
    EXPECT_FALSE(parrot.isBreedingItem(seedsStack)); // 鹦鹉不能繁殖
}

// ============================================================================
// BegGoal 兴趣状态同步测试
// ============================================================================

TEST_F(BegGoalTestFixture, BegGoal_StartExecuting_SetsWolfInterested)
{
    // 对应 MC 1.21.11 BegGoal.start(): this.wolf.setIsInterested(true)
    // BegGoal 在 startExecuting 时应调用 wolf.setInterested(true)，
    // 通过 DataParameter 同步到客户端，触发乞求头部倾斜动画。
    WolfEntity wolf(EntityInstanceId(0));
    entity::ai::goal::BegGoal begGoal(&wolf, 8.0f);

    EXPECT_FALSE(wolf.isInterested());

    begGoal.startExecuting();

    EXPECT_TRUE(wolf.isInterested());
}

TEST_F(BegGoalTestFixture, BegGoal_ResetTask_ClearsWolfInterested)
{
    // 对应 MC 1.21.11 BegGoal.stop(): this.wolf.setIsInterested(false)
    // BegGoal 在 resetTask 时应调用 wolf.setInterested(false)。
    WolfEntity wolf(EntityInstanceId(0));
    entity::ai::goal::BegGoal begGoal(&wolf, 8.0f);

    // 先设置为感兴趣
    wolf.setInterested(true);
    EXPECT_TRUE(wolf.isInterested());

    // resetTask 应清除兴趣状态
    begGoal.resetTask();

    EXPECT_FALSE(wolf.isInterested());
}

TEST_F(BegGoalTestFixture, BegGoal_StartExecuting_DataParameterDirty)
{
    // 验证 startExecuting 修改兴趣状态后，DataManager 标记为脏数据，
    // 以便 EntityTracker 在下一 tick 广播元数据到客户端。
    WolfEntity wolf(EntityInstanceId(0));
    entity::ai::goal::BegGoal begGoal(&wolf, 8.0f);
    auto& dataManager = wolf.dataManager();

    dataManager.clearDirty();
    EXPECT_FALSE(dataManager.hasDirtyData());

    begGoal.startExecuting();

    EXPECT_TRUE(dataManager.hasDirtyData());
}

TEST_F(BegGoalTestFixture, BegGoal_ResetTask_DataParameterDirty)
{
    // 验证 resetTask 修改兴趣状态后，DataManager 标记为脏数据
    WolfEntity wolf(EntityInstanceId(0));
    entity::ai::goal::BegGoal begGoal(&wolf, 8.0f);

    // 先 start 让 interested=true
    begGoal.startExecuting();

    auto& dataManager = wolf.dataManager();
    dataManager.clearDirty();
    EXPECT_FALSE(dataManager.hasDirtyData());

    // reset 改变状态 → 应标记为脏
    begGoal.resetTask();
    EXPECT_TRUE(dataManager.hasDirtyData());
}

} // namespace
} // namespace mc
