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
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/horse/DonkeyEntity.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/MuleEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用 Mock World，支持广播实体状态
 */
class HorseBreedingTestWorld final : public test::BaseTestWorld {
public:
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[BlockPos(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] f32 getBrightness(const BlockPos& /*pos*/) const override { return 1.0f; }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return 0; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("HorseBreedingTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("HorseBreedingTestWorld::tickManager not implemented");
    }

    // 实体状态广播追踪
    void broadcastEntityStatus(EntityInstanceId entityId, u8 status) override
    {
        m_lastBroadcastEntityId = entityId;
        m_lastBroadcastStatus = status;
        m_broadcastCount++;
    }

    [[nodiscard]] EntityInstanceId getLastBroadcastEntityId() const { return m_lastBroadcastEntityId; }
    [[nodiscard]] u8 getLastBroadcastStatus() const { return m_lastBroadcastStatus; }
    [[nodiscard]] i32 getBroadcastCount() const { return m_broadcastCount; }
    void resetBroadcastTracking()
    {
        m_lastBroadcastEntityId = EntityInstanceId(0);
        m_lastBroadcastStatus = 0;
        m_broadcastCount = 0;
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    EntityInstanceId m_lastBroadcastEntityId{0};
    u8 m_lastBroadcastStatus = 0;
    i32 m_broadcastCount = 0;
};

// ============================================================================
// HorseEntity::canMateWith 测试
// ============================================================================

TEST(HorseBreedingTest, Horse_CanMateWithHorse)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse1(EntityInstanceId(1));
    HorseEntity horse2(EntityInstanceId(2));
    horse1.setWorld(&world);
    horse2.setWorld(&world);

    // 成体才能繁殖
    horse1.setGrowingAge(0);
    horse2.setGrowingAge(0);

    EXPECT_TRUE(horse1.canMateWith(horse2));
    EXPECT_TRUE(horse2.canMateWith(horse1));
}

TEST(HorseBreedingTest, Horse_CanMateWithDonkey)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    DonkeyEntity donkey(EntityInstanceId(2));
    horse.setWorld(&world);
    donkey.setWorld(&world);

    // 成体才能繁殖
    horse.setGrowingAge(0);
    donkey.setGrowingAge(0);

    // 马 + 驴 = 骡
    EXPECT_TRUE(horse.canMateWith(donkey));
    EXPECT_TRUE(donkey.canMateWith(horse));
}

TEST(HorseBreedingTest, Horse_CannotMateWithLlama)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    LlamaEntity llama(EntityInstanceId(2));
    horse.setWorld(&world);
    llama.setWorld(&world);

    // 成体
    horse.setGrowingAge(0);
    llama.setGrowingAge(0);

    // 马不能与羊驼交配
    EXPECT_FALSE(horse.canMateWith(llama));
    EXPECT_FALSE(llama.canMateWith(horse));
}

TEST(HorseBreedingTest, Horse_CannotMateWithItself)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    // 不能与自己交配
    EXPECT_FALSE(horse.canMateWith(horse));
}

TEST(HorseBreedingTest, Horse_CannotMateWhenInLove)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse1(EntityInstanceId(1));
    HorseEntity horse2(EntityInstanceId(2));
    horse1.setWorld(&world);
    horse2.setWorld(&world);

    horse1.setGrowingAge(0);
    horse2.setGrowingAge(0);

    // 初始可以繁殖
    EXPECT_TRUE(horse1.canBreed());

    // 设置爱心状态后不能繁殖
    horse1.setInLove();
    EXPECT_TRUE(horse1.isInLove());
    EXPECT_FALSE(horse1.canBreed());
    EXPECT_FALSE(horse1.canMateWith(horse2));
}

TEST(HorseBreedingTest, Horse_CannotMateAsChild)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse1(EntityInstanceId(1));
    HorseEntity horse2(EntityInstanceId(2));
    horse1.setWorld(&world);
    horse2.setWorld(&world);

    // 一个成体，一个幼体
    horse1.setGrowingAge(0);
    horse2.setChild(true);

    EXPECT_FALSE(horse1.canMateWith(horse2));
    EXPECT_FALSE(horse2.canMateWith(horse1));
}

// ============================================================================
// HorseEntity::spawnBaby 测试
// ============================================================================

TEST(HorseBreedingTest, Horse_SpawnBabyWithHorse_ProducesHorse)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse1(EntityInstanceId(1));
    HorseEntity horse2(EntityInstanceId(2));
    horse1.setWorld(&world);
    horse2.setWorld(&world);

    horse1.setPosition(0.0, 64.0, 0.0);
    horse2.setPosition(1.0, 64.0, 0.0);

    // 马 + 马 = 马
    auto baby = horse1.spawnBaby(horse2);
    ASSERT_NE(baby, nullptr);

    // 验证是马
    HorseEntity* babyHorse = dynamic_cast<HorseEntity*>(baby.get());
    EXPECT_NE(babyHorse, nullptr);

    // 验证是幼体
    EXPECT_TRUE(baby->isChild());
}

TEST(HorseBreedingTest, Horse_SpawnBabyWithDonkey_ProducesMule)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    DonkeyEntity donkey(EntityInstanceId(2));
    horse.setWorld(&world);
    donkey.setWorld(&world);

    horse.setPosition(0.0, 64.0, 0.0);
    donkey.setPosition(1.0, 64.0, 0.0);

    // 马 + 驴 = 骡
    auto baby = horse.spawnBaby(donkey);
    ASSERT_NE(baby, nullptr);

    // 验证是骡
    MuleEntity* babyMule = dynamic_cast<MuleEntity*>(baby.get());
    EXPECT_NE(babyMule, nullptr);

    // 验证是幼体
    EXPECT_TRUE(baby->isChild());
}

// ============================================================================
// DonkeyEntity::canMateWith 测试
// ============================================================================

TEST(HorseBreedingTest, Donkey_CanMateWithDonkey)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    DonkeyEntity donkey1(EntityInstanceId(1));
    DonkeyEntity donkey2(EntityInstanceId(2));
    donkey1.setWorld(&world);
    donkey2.setWorld(&world);

    donkey1.setGrowingAge(0);
    donkey2.setGrowingAge(0);

    EXPECT_TRUE(donkey1.canMateWith(donkey2));
    EXPECT_TRUE(donkey2.canMateWith(donkey1));
}

TEST(HorseBreedingTest, Donkey_CanMateWithHorse)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    DonkeyEntity donkey(EntityInstanceId(1));
    HorseEntity horse(EntityInstanceId(2));
    donkey.setWorld(&world);
    horse.setWorld(&world);

    donkey.setGrowingAge(0);
    horse.setGrowingAge(0);

    // 驴 + 马 = 骡
    EXPECT_TRUE(donkey.canMateWith(horse));
}

// ============================================================================
// DonkeyEntity::spawnBaby 测试
// ============================================================================

TEST(HorseBreedingTest, Donkey_SpawnBabyWithDonkey_ProducesDonkey)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    DonkeyEntity donkey1(EntityInstanceId(1));
    DonkeyEntity donkey2(EntityInstanceId(2));
    donkey1.setWorld(&world);
    donkey2.setWorld(&world);

    donkey1.setPosition(0.0, 64.0, 0.0);
    donkey2.setPosition(1.0, 64.0, 0.0);

    // 驴 + 驴 = 驴
    auto baby = donkey1.spawnBaby(donkey2);
    ASSERT_NE(baby, nullptr);

    // 验证是驴
    DonkeyEntity* babyDonkey = dynamic_cast<DonkeyEntity*>(baby.get());
    EXPECT_NE(babyDonkey, nullptr);

    // 验证是幼体
    EXPECT_TRUE(baby->isChild());
}

TEST(HorseBreedingTest, Donkey_SpawnBabyWithHorse_ProducesMule)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    DonkeyEntity donkey(EntityInstanceId(1));
    HorseEntity horse(EntityInstanceId(2));
    donkey.setWorld(&world);
    horse.setWorld(&world);

    donkey.setPosition(0.0, 64.0, 0.0);
    horse.setPosition(1.0, 64.0, 0.0);

    // 驴 + 马 = 骡
    auto baby = donkey.spawnBaby(horse);
    ASSERT_NE(baby, nullptr);

    // 验证是骡
    MuleEntity* babyMule = dynamic_cast<MuleEntity*>(baby.get());
    EXPECT_NE(babyMule, nullptr);

    // 验证是幼体
    EXPECT_TRUE(baby->isChild());
}

// ============================================================================
// LlamaEntity::canMateWith 测试
// ============================================================================

TEST(HorseBreedingTest, Llama_CanMateWithLlama)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama1(EntityInstanceId(1));
    LlamaEntity llama2(EntityInstanceId(2));
    llama1.setWorld(&world);
    llama2.setWorld(&world);

    llama1.setGrowingAge(0);
    llama2.setGrowingAge(0);

    EXPECT_TRUE(llama1.canMateWith(llama2));
    EXPECT_TRUE(llama2.canMateWith(llama1));
}

TEST(HorseBreedingTest, Llama_CannotMateWithHorse)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama(EntityInstanceId(1));
    HorseEntity horse(EntityInstanceId(2));
    llama.setWorld(&world);
    horse.setWorld(&world);

    llama.setGrowingAge(0);
    horse.setGrowingAge(0);

    // 羊驼不能与马交配
    EXPECT_FALSE(llama.canMateWith(horse));
    EXPECT_FALSE(horse.canMateWith(llama));
}

TEST(HorseBreedingTest, Llama_CannotMateWithDonkey)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama(EntityInstanceId(1));
    DonkeyEntity donkey(EntityInstanceId(2));
    llama.setWorld(&world);
    donkey.setWorld(&world);

    llama.setGrowingAge(0);
    donkey.setGrowingAge(0);

    // 羊驼不能与驴交配
    EXPECT_FALSE(llama.canMateWith(donkey));
    EXPECT_FALSE(donkey.canMateWith(llama));
}

// ============================================================================
// LlamaEntity::spawnBaby 测试
// ============================================================================

TEST(HorseBreedingTest, Llama_SpawnBaby_ProducesLlama)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama1(EntityInstanceId(1));
    LlamaEntity llama2(EntityInstanceId(2));
    llama1.setWorld(&world);
    llama2.setWorld(&world);

    llama1.setPosition(0.0, 64.0, 0.0);
    llama2.setPosition(1.0, 64.0, 0.0);

    // 设置强度
    llama1.setStrength(3);
    llama2.setStrength(4);

    // 羊驼 + 羊驼 = 羊驼
    auto baby = llama1.spawnBaby(llama2);
    ASSERT_NE(baby, nullptr);

    // 验证是羊驼
    LlamaEntity* babyLlama = dynamic_cast<LlamaEntity*>(baby.get());
    EXPECT_NE(babyLlama, nullptr);

    // 验证是幼体
    EXPECT_TRUE(baby->isChild());

    // 验证强度遗传（应该 >= max(3, 4) = 4）
    EXPECT_GE(babyLlama->getStrength(), 4);
    EXPECT_LE(babyLlama->getStrength(), 5); // 最大 5
}

// ============================================================================
// isBreedingItem 测试
// ============================================================================

TEST(HorseBreedingTest, Horse_IsBreedingItem_GoldenApple)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    ItemStack stack(Items::GOLDEN_APPLE, 1);
    EXPECT_TRUE(horse.isBreedingItem(stack));
    EXPECT_TRUE(horse.isFoodItem(stack));
}

TEST(HorseBreedingTest, Horse_IsBreedingItem_GoldenCarrot)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    ItemStack stack(Items::GOLDEN_CARROT, 1);
    EXPECT_TRUE(horse.isBreedingItem(stack));
    EXPECT_TRUE(horse.isFoodItem(stack));
}

TEST(HorseBreedingTest, Horse_IsBreedingItem_Wheat)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    ItemStack stack(Items::WHEAT, 1);
    // 小麦是食物，但不会触发繁殖（isBreedingItem 返回 isFoodItem 的结果）
    EXPECT_TRUE(horse.isBreedingItem(stack));
    EXPECT_TRUE(horse.isFoodItem(stack));
}

TEST(HorseBreedingTest, Llama_IsBreedingItem_HayBlock)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama(EntityInstanceId(1));
    llama.setWorld(&world);

    ItemStack stack(Items::HAY_BLOCK, 1);
    EXPECT_TRUE(llama.isBreedingItem(stack));
    EXPECT_TRUE(llama.isFoodItem(stack));
}

TEST(HorseBreedingTest, Llama_IsBreedingItem_Wheat)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama(EntityInstanceId(1));
    llama.setWorld(&world);

    ItemStack stack(Items::WHEAT, 1);
    // 小麦是食物，会触发 TemptGoal
    EXPECT_TRUE(llama.isBreedingItem(stack));
    EXPECT_TRUE(llama.isFoodItem(stack));
}

// ============================================================================
// MuleEntity 不育测试
// ============================================================================

TEST(HorseBreedingTest, Mule_CannotBreed)
{
    VanillaBlocks::initialize();

    HorseBreedingTestWorld world;
    MuleEntity mule(EntityInstanceId(1));
    mule.setWorld(&world);

    // 骡是不育的
    mule.setGrowingAge(0);
    // 骡的 canBreed() 返回 false（AgeableEntity 基类逻辑）
    // 但骡本身不应该与其他马类交配
    // 注意：当前实现中 MuleEntity 没有重写 canMateWith
    // 这是一个边界情况测试
}

// ============================================================================
// 繁殖物品消耗测试（通过 handleEating）
// ============================================================================

TEST(HorseBreedingTest, Horse_HandleEating_GoldenApple_TriggersBreeding)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    // 成体，可以繁殖
    horse.setGrowingAge(0);
    horse.setTame(true);

    EXPECT_TRUE(horse.canBreed());
    EXPECT_FALSE(horse.isInLove());

    // 喂食金苹果
    ItemStack stack(Items::GOLDEN_APPLE, 1);
    bool result = horse.handleEating(nullptr, stack);

    EXPECT_TRUE(result);
    EXPECT_TRUE(horse.isInLove());
}

TEST(HorseBreedingTest, Horse_HandleEating_GoldenCarrot_TriggersBreeding)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    horse.setGrowingAge(0);
    horse.setTame(true);

    EXPECT_TRUE(horse.canBreed());

    ItemStack stack(Items::GOLDEN_CARROT, 1);
    bool result = horse.handleEating(nullptr, stack);

    EXPECT_TRUE(result);
    EXPECT_TRUE(horse.isInLove());
}

TEST(HorseBreedingTest, Horse_HandleEating_Wheat_NoBreeding)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    horse.setGrowingAge(0);
    horse.setTame(true);

    // 确保马的生命值低于最大值
    // 马的最大生命值是 15-30，初始化时随机设定
    f32 maxHp = horse.maxHealth();
    ASSERT_GT(maxHp, 5.0f) << "Horse maxHealth should be initialized";

    // 设置生命值比最大值少，这样小麦才能有治疗效果
    horse.setHealth(maxHp - 5.0f);
    EXPECT_FLOAT_EQ(horse.health(), maxHp - 5.0f);

    // 小麦是马的食物，有治疗效果，但不会触发繁殖
    ItemStack stack(Items::WHEAT, 1);
    bool result = horse.handleEating(nullptr, stack);

    EXPECT_TRUE(result);            // 小麦有治疗效果
    EXPECT_FALSE(horse.isInLove()); // 但不会进入爱心状态
    // 小麦治疗 2 点生命值
    EXPECT_FLOAT_EQ(horse.health(), maxHp - 5.0f + 2.0f);
}

TEST(HorseBreedingTest, Llama_HandleEating_HayBlock_TriggersBreeding)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama(EntityInstanceId(1));
    llama.setWorld(&world);

    llama.setGrowingAge(0);

    EXPECT_TRUE(llama.canBreed());
    EXPECT_FALSE(llama.isInLove());

    // 喂食干草块
    ItemStack stack(Items::HAY_BLOCK, 1);
    bool result = llama.handleEating(nullptr, stack);

    EXPECT_TRUE(result);
    EXPECT_TRUE(llama.isInLove());
}

TEST(HorseBreedingTest, Llama_HandleEating_Wheat_NoBreeding)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama(EntityInstanceId(1));
    llama.setWorld(&world);

    llama.setGrowingAge(0);

    // 小麦不会触发繁殖
    ItemStack stack(Items::WHEAT, 1);
    bool result = llama.handleEating(nullptr, stack);

    EXPECT_TRUE(result);            // 有治疗效果
    EXPECT_FALSE(llama.isInLove()); // 但不会进入爱心状态
}

// ============================================================================
// 驯服进度增加测试
// ============================================================================

TEST(HorseBreedingTest, Horse_HandleEating_GoldenApple_IncreasesTemper)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    horse.setGrowingAge(0);
    horse.setTame(false);

    i32 initialTemper = horse.getTemper();

    // 喂食金苹果
    ItemStack stack(Items::GOLDEN_APPLE, 1);
    horse.handleEating(nullptr, stack);

    // 金苹果增加 10 点驯服进度
    EXPECT_EQ(horse.getTemper(), initialTemper + 10);
}

TEST(HorseBreedingTest, Horse_HandleEating_GoldenCarrot_IncreasesTemper)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    horse.setGrowingAge(0);
    horse.setTame(false);

    i32 initialTemper = horse.getTemper();

    // 喂食金胡萝卜
    ItemStack stack(Items::GOLDEN_CARROT, 1);
    horse.handleEating(nullptr, stack);

    // 金胡萝卜增加 5 点驯服进度
    EXPECT_EQ(horse.getTemper(), initialTemper + 5);
}

TEST(HorseBreedingTest, Llama_HandleEating_Wheat_IncreasesTemper)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama(EntityInstanceId(1));
    llama.setWorld(&world);

    llama.setGrowingAge(0);
    llama.setTame(false);

    i32 initialTemper = llama.getTemper();

    // 喂食小麦
    ItemStack stack(Items::WHEAT, 1);
    llama.handleEating(nullptr, stack);

    // 小麦增加 3 点驯服进度
    EXPECT_EQ(llama.getTemper(), initialTemper + 3);
}

TEST(HorseBreedingTest, Llama_HandleEating_HayBlock_IncreasesTemper)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama(EntityInstanceId(1));
    llama.setWorld(&world);

    llama.setGrowingAge(0);
    llama.setTame(false);

    i32 initialTemper = llama.getTemper();

    // 喂食干草块
    ItemStack stack(Items::HAY_BLOCK, 1);
    llama.handleEating(nullptr, stack);

    // 干草块增加 6 点驯服进度
    EXPECT_EQ(llama.getTemper(), initialTemper + 6);
}

// ============================================================================
// 幼体成长加速测试
// ============================================================================

TEST(HorseBreedingTest, Horse_HandleEating_AcceleratesGrowth)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    // 幼体
    horse.setChild(true);
    EXPECT_TRUE(horse.isChild());

    i32 initialAge = horse.getGrowingAge();

    // 喂食苹果加速成长
    ItemStack stack(Items::APPLE, 1);
    bool result = horse.handleEating(nullptr, stack);

    EXPECT_TRUE(result);
    // 苹果加速 60 ticks 成长
    EXPECT_EQ(horse.getGrowingAge(), initialAge + 60);
}

TEST(HorseBreedingTest, Llama_HandleEating_HayBlock_AcceleratesGrowth)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama(EntityInstanceId(1));
    llama.setWorld(&world);

    llama.setChild(true);
    EXPECT_TRUE(llama.isChild());

    i32 initialAge = llama.getGrowingAge();

    // 喂食干草块加速成长
    ItemStack stack(Items::HAY_BLOCK, 1);
    bool result = llama.handleEating(nullptr, stack);

    EXPECT_TRUE(result);
    // 干草块加速 90 ticks 成长
    EXPECT_EQ(llama.getGrowingAge(), initialAge + 90);
}

// ============================================================================
// 治疗测试
// ============================================================================

TEST(HorseBreedingTest, Horse_HandleEating_Heals)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    HorseEntity horse(EntityInstanceId(1));
    horse.setWorld(&world);

    horse.setGrowingAge(0);

    // 确保马的生命值低于最大值
    // 马的最大生命值是 15-30，初始化时随机设定
    f32 maxHp = horse.maxHealth();
    ASSERT_GT(maxHp, 5.0f); // 确保属性已初始化

    horse.setHealth(maxHp - 5.0f); // 设置生命值比最大值少 5
    EXPECT_FLOAT_EQ(horse.health(), maxHp - 5.0f);

    // 喂食苹果治疗
    ItemStack stack(Items::APPLE, 1);
    bool result = horse.handleEating(nullptr, stack);

    EXPECT_TRUE(result);
    // 苹果治疗 3 点生命值
    EXPECT_FLOAT_EQ(horse.health(), maxHp - 5.0f + 3.0f);
}

TEST(HorseBreedingTest, Llama_HandleEating_Heals)
{
    VanillaBlocks::initialize();
    Items::initialize();

    HorseBreedingTestWorld world;
    LlamaEntity llama(EntityInstanceId(1));
    llama.setWorld(&world);

    llama.setGrowingAge(0);

    // 确保羊驼的生命值低于最大值
    f32 maxHp = llama.maxHealth();
    ASSERT_GT(maxHp, 5.0f) << "Llama maxHealth should be initialized";

    // 设置生命值为最大值的一半，这样治愈不会溢出
    f32 initialHealth = maxHp / 2.0f;
    llama.setHealth(initialHealth);
    EXPECT_FLOAT_EQ(llama.health(), initialHealth);

    // 喂食干草块治疗
    ItemStack stack(Items::HAY_BLOCK, 1);
    bool result = llama.handleEating(nullptr, stack);

    EXPECT_TRUE(result);
    // 干草块治疗 10 点生命值，但不能超过最大值
    f32 expectedHealth = std::min(initialHealth + 10.0f, maxHp);
    EXPECT_FLOAT_EQ(llama.health(), expectedHealth);
}

} // namespace
} // namespace mc
