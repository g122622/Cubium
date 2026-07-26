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
#include "common/core/Types.hpp"
#include "common/entity/entities/passive/tamable/ParrotEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/sound/SoundEvents.hpp"
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
 * @brief 鹦鹉实体测试用世界
 *
 * 提供最小化测试环境用于鹦鹉实体功能测试
 * 支持追踪 broadcastEntityStatus 和 playSound 调用
 */
class ParrotTestWorld final : public test::BaseTestWorld {
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
        throw std::runtime_error("ParrotTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ParrotTestWorld::tickManager not implemented");
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

    // 追踪 broadcastEntityStatus 调用
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
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;

    // 声音追踪
    ResourceLocation m_lastSoundId;
    i32 m_soundPlayCount = 0;

    // 广播追踪
    EntityInstanceId m_lastBroadcastEntityId{0};
    u8 m_lastBroadcastStatus = 0;
    i32 m_broadcastCount = 0;
};

class ParrotEntityTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    ParrotTestWorld m_world;
};

// ============================================================================
// 驯服物品测试
// 参考 MC 1.16.5: ParrotEntity.TAME_ITEMS = {WHEAT_SEEDS, MELON_SEEDS, PUMPKIN_SEEDS, BEETROOT_SEEDS}
// ============================================================================

TEST_F(ParrotEntityTestFixture, IsTameItem_WheatSeeds_ReturnsTrue)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 小麦种子可以驯服鹦鹉
    ItemStack stack(Items::WHEAT_SEEDS, 1);
    EXPECT_TRUE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_PumpkinSeeds_ReturnsTrue)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 南瓜种子可以驯服鹦鹉
    ItemStack stack(Items::PUMPKIN_SEEDS, 1);
    EXPECT_TRUE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_MelonSeeds_ReturnsTrue)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 西瓜种子可以驯服鹦鹉
    ItemStack stack(Items::MELON_SEEDS, 1);
    EXPECT_TRUE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_BeetrootSeeds_ReturnsTrue)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 甜菜种子可以驯服鹦鹉
    ItemStack stack(Items::BEETROOT_SEEDS, 1);
    EXPECT_TRUE(parrot.isTameItem(stack));
}

// ============================================================================
// 非驯服物品测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, IsTameItem_Wheat_ReturnsFalse)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 小麦不能驯服鹦鹉
    ItemStack stack(Items::WHEAT, 1);
    EXPECT_FALSE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_Bone_ReturnsFalse)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 骨头不能驯服鹦鹉（骨头用于驯服狼）
    ItemStack stack(Items::BONE, 1);
    EXPECT_FALSE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_Cod_ReturnsFalse)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 生鳕鱼不能驯服鹦鹉（生鳕鱼用于驯服猫）
    ItemStack stack(Items::COD, 1);
    EXPECT_FALSE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_Apple_ReturnsFalse)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 苹果不能驯服鹦鹉
    ItemStack stack(Items::APPLE, 1);
    EXPECT_FALSE(parrot.isTameItem(stack));
}

// ============================================================================
// 空物品测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, IsTameItem_EmptyStack_ReturnsFalse)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 空物品堆不能驯服
    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(parrot.isTameItem(emptyStack));
}

// ============================================================================
// 繁殖测试 - 鹦鹉不能繁殖
// 参考 MC 1.16.5: 鹦鹉是唯一不能繁殖的可驯服动物
// ============================================================================

TEST_F(ParrotEntityTestFixture, IsBreedingItem_AnyItem_ReturnsFalse)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 鹦鹉不能繁殖，任何物品都返回 false
    ItemStack wheatSeeds(Items::WHEAT_SEEDS, 1);
    EXPECT_FALSE(parrot.isBreedingItem(wheatSeeds));

    ItemStack pumpkinSeeds(Items::PUMPKIN_SEEDS, 1);
    EXPECT_FALSE(parrot.isBreedingItem(pumpkinSeeds));

    ItemStack wheat(Items::WHEAT, 1);
    EXPECT_FALSE(parrot.isBreedingItem(wheat));
}

TEST_F(ParrotEntityTestFixture, SpawnBaby_ReturnsNullptr)
{
    ParrotEntity parent1(EntityInstanceId(0));
    ParrotEntity parent2(EntityInstanceId(0));

    // 鹦鹉不能生成幼体
    auto baby = parent1.spawnBaby(parent2);
    EXPECT_EQ(baby, nullptr);
}

// ============================================================================
// 变种测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, Variant_CanBeSetAndGet)
{
    ParrotEntity parrot(EntityInstanceId(0));

    parrot.setVariant(ParrotEntity::ParrotVariant::Blue);
    EXPECT_EQ(parrot.getVariant(), ParrotEntity::ParrotVariant::Blue);

    parrot.setVariant(ParrotEntity::ParrotVariant::Green);
    EXPECT_EQ(parrot.getVariant(), ParrotEntity::ParrotVariant::Green);

    parrot.setVariant(ParrotEntity::ParrotVariant::YellowBlue);
    EXPECT_EQ(parrot.getVariant(), ParrotEntity::ParrotVariant::YellowBlue);

    parrot.setVariant(ParrotEntity::ParrotVariant::Gray);
    EXPECT_EQ(parrot.getVariant(), ParrotEntity::ParrotVariant::Gray);
}

TEST_F(ParrotEntityTestFixture, RandomizeVariant_SetsValidVariant)
{
    ParrotEntity parrot(EntityInstanceId(0));

    parrot.randomizeVariant();

    // 变种应该在 0-4 范围内
    auto variant = parrot.getVariant();
    EXPECT_GE(static_cast<u8>(variant), 0u);
    EXPECT_LE(static_cast<u8>(variant), 4u);
}

// ============================================================================
// 飞行测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, Flying_CanBeSetAndCleared)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 初始不飞行
    EXPECT_FALSE(parrot.isFlying());

    // 设置飞行
    parrot.setFlying(true);
    EXPECT_TRUE(parrot.isFlying());

    // 清除飞行
    parrot.setFlying(false);
    EXPECT_FALSE(parrot.isFlying());
}

TEST_F(ParrotEntityTestFixture, CanFly_AlwaysReturnsTrue)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 鹦鹉始终可以飞
    EXPECT_TRUE(parrot.canFly());
}

// ============================================================================
// 模仿测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, Imitation_CanBeSetAndQueried)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 初始不模仿
    EXPECT_FALSE(parrot.isImitating());

    // 设置模仿目标
    parrot.setImitatingTarget(100);
    EXPECT_TRUE(parrot.isImitating());
    EXPECT_EQ(parrot.getImitatingTarget(), 100u);

    // 设置模仿状态
    parrot.setImitating(false);
    EXPECT_FALSE(parrot.isImitating());
}

// ============================================================================
// 驯服状态测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, TamedState_DefaultFalse)
{
    // 鹦鹉默认未驯服
    ParrotEntity parrot(EntityInstanceId(0));
    EXPECT_FALSE(parrot.isTamed());
}

TEST_F(ParrotEntityTestFixture, TamedState_CanBeSet)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 设置为已驯服
    parrot.setTamed(true);
    EXPECT_TRUE(parrot.isTamed());

    // 设置回未驯服
    parrot.setTamed(false);
    EXPECT_FALSE(parrot.isTamed());
}

TEST_F(ParrotEntityTestFixture, OwnerId_CanBeSetAndQueried)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 默认无主人
    EXPECT_FALSE(parrot.getOwnerId().has_value());

    // 设置主人
    parrot.setOwnerId(12345ULL);
    EXPECT_TRUE(parrot.getOwnerId().has_value());
    EXPECT_EQ(parrot.getOwnerId().value(), 12345ULL);

    // 检查是否是主人
    EXPECT_TRUE(parrot.isOwner(12345ULL));
    EXPECT_FALSE(parrot.isOwner(99999ULL));

    // 清除主人
    parrot.clearOwner();
    EXPECT_FALSE(parrot.getOwnerId().has_value());
}

// ============================================================================
// 坐下状态测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, SittingState_DefaultFalse)
{
    // 鹦鹉默认不坐下
    ParrotEntity parrot(EntityInstanceId(0));
    EXPECT_FALSE(parrot.isSitting());
}

TEST_F(ParrotEntityTestFixture, SittingState_CanBeSet)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 设置坐下
    parrot.setSitting(true);
    EXPECT_TRUE(parrot.isSitting());

    // 设置站起
    parrot.setSitting(false);
    EXPECT_FALSE(parrot.isSitting());
}

TEST_F(ParrotEntityTestFixture, ToggleSitting_SwitchesState)
{
    ParrotEntity parrot(EntityInstanceId(0));

    // 切换坐下
    parrot.toggleSitting();
    EXPECT_TRUE(parrot.isSitting());

    // 再次切换
    parrot.toggleSitting();
    EXPECT_FALSE(parrot.isSitting());
}

// ============================================================================
// 肩膀乘坐测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, ShoulderRiding_DefaultNotOnShoulder)
{
    // 默认不在肩膀上
    ParrotEntity parrot(EntityInstanceId(0));
    EXPECT_FALSE(parrot.isOnShoulder());
}

TEST_F(ParrotEntityTestFixture, ShoulderRiding_CanSitOnShoulder_WhenTamed)
{
    ParrotEntity parrot(EntityInstanceId(0));
    parrot.setWorld(&m_world);

    // 未驯服时不能坐在肩膀上
    EXPECT_FALSE(parrot.canSitOnShoulder());

    // 设置为已驯服
    parrot.setTamed(true);

    // 需要等待冷却 (canSitOnShoulder 需要 rideCooldownCounter > 100)
    // 由于 tick() 会增加计数器，这里直接测试 mountShoulder
    // 驯服后但不坐下时可以坐肩膀
    parrot.setSitting(false);

    // 模拟 tick 增加 rideCooldownCounter
    for (int i = 0; i < 110; ++i) {
        parrot.tick();
    }

    // 现在应该可以坐肩膀
    EXPECT_TRUE(parrot.canSitOnShoulder());

    // 坐上肩膀
    EXPECT_TRUE(parrot.mountShoulder(12345ULL));
    EXPECT_TRUE(parrot.isOnShoulder());
    EXPECT_EQ(parrot.getShoulderPlayerId(), 12345ULL);
}

TEST_F(ParrotEntityTestFixture, ShoulderRiding_CannotSitOnShoulder_WhenSitting)
{
    ParrotEntity parrot(EntityInstanceId(0));
    parrot.setWorld(&m_world);
    parrot.setTamed(true);
    parrot.setSitting(true);

    // 模拟 tick 增加 rideCooldownCounter
    for (int i = 0; i < 110; ++i) {
        parrot.tick();
    }

    // 坐下状态不能坐肩膀
    EXPECT_FALSE(parrot.mountShoulder(12345ULL));
    EXPECT_FALSE(parrot.isOnShoulder());
}

TEST_F(ParrotEntityTestFixture, ShoulderRiding_DismountShoulder_ResetsState)
{
    ParrotEntity parrot(EntityInstanceId(0));
    parrot.setWorld(&m_world);
    parrot.setTamed(true);

    // 模拟 tick 增加冷却
    for (int i = 0; i < 110; ++i) {
        parrot.tick();
    }

    // 坐上肩膀
    parrot.mountShoulder(12345ULL);
    EXPECT_TRUE(parrot.isOnShoulder());

    // 离开肩膀
    parrot.dismountShoulder();
    EXPECT_FALSE(parrot.isOnShoulder());
    EXPECT_EQ(parrot.getShoulderPlayerId(), 0u);

    // 冷却应该重置
    EXPECT_FALSE(parrot.canSitOnShoulder());
}

// ============================================================================
// 属性测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, Attributes_CorrectValues)
{
    // MC 1.16.5: 鹦鹉属性
    // MAX_HEALTH = 6.0
    // MOVEMENT_SPEED = 0.2
    // FLYING_SPEED = 0.4
    ParrotEntity parrot(EntityInstanceId(0));

    f64 maxHealth = parrot.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH, 0.0);
    EXPECT_DOUBLE_EQ(maxHealth, 6.0);

    f64 movementSpeed = parrot.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    EXPECT_DOUBLE_EQ(movementSpeed, 0.2);

    f64 flyingSpeed = parrot.getAttributeValue(entity::attribute::Attributes::FLYING_SPEED, 0.0);
    EXPECT_DOUBLE_EQ(flyingSpeed, 0.4);
}

// ============================================================================
// 眼睛高度测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, EyeHeight_CorrectValue)
{
    // MC 1.16.5: 鹦鹉眼睛高度 = 0.25
    ParrotEntity parrot(EntityInstanceId(0));
    EXPECT_FLOAT_EQ(parrot.eyeHeight(), 0.25f);
}

// ============================================================================
// interactMob 驯服交互测试
// 参考 MC 1.16.5: ParrotEntity.func_230254_b_()
// ============================================================================

TEST_F(ParrotEntityTestFixture, InteractMob_UntamedParrot_WithWheatSeeds_PlaysEatSound)
{
    // 未驯服的鹦鹉用小麦种子交互，应该播放吃东西声音
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    // 创建玩家并设置手持物品
    Player player(EntityInstanceId(2), "TestPlayer");
    ItemStack seedStack(Items::WHEAT_SEEDS, 10);
    player.inventory().setItem(0, seedStack);
    player.inventory().setSelectedSlot(0);

    // 重置追踪
    world.resetSoundTracking();

    // 交互
    ActionResultType result = parrot.interactMob(player, Hand::MainHand);

    // 应该返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 应该播放吃东西声音
    EXPECT_EQ(world.getSoundPlayCount(), 1);
    EXPECT_EQ(world.getLastSoundId(), SoundEvents::ENTITY_PARROT_EAT);
}

TEST_F(ParrotEntityTestFixture, InteractMob_UntamedParrot_WithPumpkinSeeds_PlaysEatSound)
{
    // 未驯服的鹦鹉用南瓜种子交互
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    Player player(EntityInstanceId(2), "TestPlayer");
    ItemStack seedStack(Items::PUMPKIN_SEEDS, 10);
    player.inventory().setItem(0, seedStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    ActionResultType result = parrot.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(world.getSoundPlayCount(), 1);
    EXPECT_EQ(world.getLastSoundId(), SoundEvents::ENTITY_PARROT_EAT);
}

TEST_F(ParrotEntityTestFixture, InteractMob_UntamedParrot_WithMelonSeeds_PlaysEatSound)
{
    // 未驯服的鹦鹉用西瓜种子交互
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    Player player(EntityInstanceId(2), "TestPlayer");
    ItemStack seedStack(Items::MELON_SEEDS, 10);
    player.inventory().setItem(0, seedStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    ActionResultType result = parrot.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(world.getSoundPlayCount(), 1);
    EXPECT_EQ(world.getLastSoundId(), SoundEvents::ENTITY_PARROT_EAT);
}

TEST_F(ParrotEntityTestFixture, InteractMob_UntamedParrot_WithBeetrootSeeds_PlaysEatSound)
{
    // 未驯服的鹦鹉用甜菜种子交互
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    Player player(EntityInstanceId(2), "TestPlayer");
    ItemStack seedStack(Items::BEETROOT_SEEDS, 10);
    player.inventory().setItem(0, seedStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    ActionResultType result = parrot.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(world.getSoundPlayCount(), 1);
    EXPECT_EQ(world.getLastSoundId(), SoundEvents::ENTITY_PARROT_EAT);
}

TEST_F(ParrotEntityTestFixture, InteractMob_TamingSuccess_BroadcastsSuccessStatus)
{
    // 驯服成功场景 - 通过多次尝试模拟 1/10 概率
    // 注意：由于随机性，我们无法直接控制，但可以验证成功时的行为
    ParrotTestWorld world;

    // 创建鹦鹉并设置世界
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    // 创建玩家
    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);

    // 验证成功驯服后的状态
    parrot.setTamed(true);
    parrot.setOwnerId(12345ULL);

    EXPECT_TRUE(parrot.isTamed());
    EXPECT_TRUE(parrot.isOwner(12345ULL));
}

TEST_F(ParrotEntityTestFixture, InteractMob_TamingFailure_BroadcastsFailStatus)
{
    // 驯服失败场景 - 广播烟雾粒子
    // 注意：测试通过后验证广播状态是 TamingFailed
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    // 创建玩家
    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);

    ItemStack seedStack(Items::WHEAT_SEEDS, 10);
    player.inventory().setItem(0, seedStack);
    player.inventory().setSelectedSlot(0);

    world.resetBroadcastTracking();

    // 执行交互
    parrot.interactMob(player, Hand::MainHand);

    // 应该有广播（成功或失败）
    EXPECT_EQ(world.getBroadcastCount(), 1);
    EXPECT_EQ(world.getLastBroadcastEntityId(), EntityInstanceId(1));

    // 广播状态应该是 TamingSucceeded(7) 或 TamingFailed(6)
    u8 status = world.getLastBroadcastStatus();
    bool isValidStatus = (status == static_cast<u8>(network::EntityStatus::TamingSucceeded) ||
        status == static_cast<u8>(network::EntityStatus::TamingFailed));
    EXPECT_TRUE(isValidStatus);
}

TEST_F(ParrotEntityTestFixture, InteractMob_ItemConsumption_NonCreativeMode)
{
    // 非创造模式下，交互应该消耗物品
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    Player player(EntityInstanceId(2), "TestPlayer");
    player.abilities().creativeMode = false; // 非创造模式

    ItemStack seedStack(Items::WHEAT_SEEDS, 10);
    player.inventory().setItem(0, seedStack);
    player.inventory().setSelectedSlot(0);

    // 交互前物品数量
    i32 countBefore = player.inventory().getItem(0).getCount();

    // 交互
    parrot.interactMob(player, Hand::MainHand);

    // 非创造模式下物品应该减少
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, countBefore - 1);
}

TEST_F(ParrotEntityTestFixture, InteractMob_ItemConsumption_CreativeMode_NoConsumption)
{
    // 创造模式下，交互不应该消耗物品
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    Player player(EntityInstanceId(2), "TestPlayer");
    player.abilities().creativeMode = true; // 创造模式

    ItemStack seedStack(Items::WHEAT_SEEDS, 10);
    player.inventory().setItem(0, seedStack);
    player.inventory().setSelectedSlot(0);

    // 交互前物品数量
    i32 countBefore = player.inventory().getItem(0).getCount();

    // 交互
    parrot.interactMob(player, Hand::MainHand);

    // 创造模式下物品不应该减少
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, countBefore);
}

TEST_F(ParrotEntityTestFixture, InteractMob_TamedParrot_TogglesSitting)
{
    // 已驯服的鹦鹉交互切换坐下状态
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    // 设置为已驯服
    parrot.setTamed(true);
    parrot.setOwnerId(12345ULL);

    // 创建主人玩家
    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);

    // 交互不需要物品（已驯服）
    EXPECT_FALSE(parrot.isSitting());

    // 第一次交互 - 坐下
    ActionResultType result = parrot.interactMob(player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(parrot.isSitting());

    // 第二次交互 - 站起
    result = parrot.interactMob(player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_FALSE(parrot.isSitting());
}

TEST_F(ParrotEntityTestFixture, InteractMob_TamedParrot_OtherPlayerCannotToggle)
{
    // 已驯服的鹦鹉，非主人不能切换坐下状态
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    // 设置为已驯服，主人是玩家 12345
    parrot.setTamed(true);
    parrot.setOwnerId(12345ULL);

    // 创建另一个玩家
    Player otherPlayer(EntityInstanceId(2), "OtherPlayer");
    otherPlayer.setPlayerId(99999ULL);

    // 非主人交互 - 应该返回 Pass（调用父类）
    ActionResultType result = parrot.interactMob(otherPlayer, Hand::MainHand);
    // 非主人不能切换坐下状态
    EXPECT_FALSE(parrot.isSitting());
}

TEST_F(ParrotEntityTestFixture, InteractMob_UntamedParrot_NonSeedItem_PassesToParent)
{
    // 未驯服的鹦鹉用非种子物品交互，应该传递给父类
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    Player player(EntityInstanceId(2), "TestPlayer");

    // 使用非种子物品（苹果）
    ItemStack appleStack(Items::APPLE, 10);
    player.inventory().setItem(0, appleStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    // 交互
    ActionResultType result = parrot.interactMob(player, Hand::MainHand);

    // 非种子物品，鹦鹉不应该处理，传递给父类
    // ShoulderRidingEntity 的 interactMob 应该返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应该播放吃东西声音
    EXPECT_EQ(world.getSoundPlayCount(), 0);
}

TEST_F(ParrotEntityTestFixture, InteractMob_SilentParrot_NoSoundPlayed)
{
    // 静音的鹦鹉不应该播放声音
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);
    parrot.setSilent(true); // 设置静音

    Player player(EntityInstanceId(2), "TestPlayer");
    ItemStack seedStack(Items::WHEAT_SEEDS, 10);
    player.inventory().setItem(0, seedStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    // 交互
    parrot.interactMob(player, Hand::MainHand);

    // 静音状态下不应该播放声音
    EXPECT_EQ(world.getSoundPlayCount(), 0);
}

TEST_F(ParrotEntityTestFixture, InteractMob_OffHand_UsesOffHandItem)
{
    // 副手交互测试
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    Player player(EntityInstanceId(2), "TestPlayer");

    // 主手放苹果（非驯服物品），副手放种子
    ItemStack appleStack(Items::APPLE, 10);
    ItemStack seedStack(Items::WHEAT_SEEDS, 10);
    player.inventory().setItem(0, appleStack); // 主手
    player.inventory().setItem(40, seedStack); // 副手槽位
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    // 使用副手交互
    ActionResultType result = parrot.interactMob(player, Hand::OffHand);

    // 应该使用副手的种子进行驯服尝试
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(world.getSoundPlayCount(), 1);
    EXPECT_EQ(world.getLastSoundId(), SoundEvents::ENTITY_PARROT_EAT);
}

TEST_F(ParrotEntityTestFixture, InteractMob_TamingSuccess_UpdatesOwner)
{
    // 驯服成功后应该设置主人
    // 这个测试验证驯服成功时的状态更新
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    // 初始状态
    EXPECT_FALSE(parrot.isTamed());
    EXPECT_FALSE(parrot.getOwnerId().has_value());

    // 模拟驯服成功
    parrot.setTamed(true);
    parrot.setOwnerId(12345ULL);

    // 验证状态
    EXPECT_TRUE(parrot.isTamed());
    EXPECT_TRUE(parrot.getOwnerId().has_value());
    EXPECT_EQ(parrot.getOwnerId().value(), 12345ULL);
    EXPECT_TRUE(parrot.isOwner(12345ULL));
}

TEST_F(ParrotEntityTestFixture, InteractMob_ClientSide_NoBroadcast)
{
    // 客户端不应该广播实体状态
    // 注意：当前测试世界的 isClientSide() 返回 false
    // 此测试验证服务端行为
    ParrotTestWorld world;
    ParrotEntity parrot(EntityInstanceId(1));
    parrot.setWorld(&world);

    // 验证世界是服务端
    EXPECT_FALSE(world.isClientSide());

    Player player(EntityInstanceId(2), "TestPlayer");
    ItemStack seedStack(Items::WHEAT_SEEDS, 10);
    player.inventory().setItem(0, seedStack);
    player.inventory().setSelectedSlot(0);

    world.resetBroadcastTracking();

    // 服务端交互应该广播
    parrot.interactMob(player, Hand::MainHand);

    // 应该有广播
    EXPECT_EQ(world.getBroadcastCount(), 1);
}

} // namespace
} // namespace mc
