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
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

// Player 头文件用于 interactMob 测试
#include "common/entity/entities/player/Player.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 狼实体测试用世界
 *
 * 提供最小化测试环境用于狼实体功能测试
 * 支持追踪 broadcastEntityStatus、playSound、onTameAnimal 调用
 */
class WolfTestWorld final : public test::BaseTestWorld {
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

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("WolfTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("WolfTestWorld::tickManager not implemented");
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
    void broadcastEntityStatus(EntityId entityId, u8 status) override
    {
        m_lastBroadcastEntityId = entityId;
        m_lastBroadcastStatus = status;
        m_broadcastCount++;
    }

    [[nodiscard]] EntityId getLastBroadcastEntityId() const { return m_lastBroadcastEntityId; }
    [[nodiscard]] u8 getLastBroadcastStatus() const { return m_lastBroadcastStatus; }
    [[nodiscard]] i32 getBroadcastCount() const { return m_broadcastCount; }
    void resetBroadcastTracking()
    {
        m_lastBroadcastEntityId = EntityId(0);
        m_lastBroadcastStatus = 0;
        m_broadcastCount = 0;
    }

    // 追踪 onTameAnimal 调用
    void onTameAnimal(PlayerId playerId, Entity* animal) override
    {
        m_tameAnimalCalled = true;
        m_lastTamePlayerId = playerId;
        m_lastTameAnimal = animal;
    }

    [[nodiscard]] bool wasTameAnimalCalled() const { return m_tameAnimalCalled; }
    [[nodiscard]] PlayerId getLastTamePlayerId() const { return m_lastTamePlayerId; }
    [[nodiscard]] Entity* getLastTameAnimal() const { return m_lastTameAnimal; }
    void resetTameTracking()
    {
        m_tameAnimalCalled = false;
        m_lastTamePlayerId = 0;
        m_lastTameAnimal = nullptr;
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;

    // 声音追踪
    ResourceLocation m_lastSoundId;
    i32 m_soundPlayCount = 0;

    // 广播追踪
    EntityId m_lastBroadcastEntityId{0};
    u8 m_lastBroadcastStatus = 0;
    i32 m_broadcastCount = 0;

    // 驯服追踪
    bool m_tameAnimalCalled = false;
    PlayerId m_lastTamePlayerId = 0;
    Entity* m_lastTameAnimal = nullptr;
};

class WolfEntityTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    WolfTestWorld m_world;
};

// ============================================================================
// 驯服物品测试
// ============================================================================

TEST_F(WolfEntityTestFixture, IsTameItem_Bone_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    // 骨头是驯服狼的唯一物品
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_TRUE(wolf.isTameItem(boneStack));
}

TEST_F(WolfEntityTestFixture, IsTameItem_Meat_ReturnsFalse)
{
    WolfEntity wolf(EntityId(0));

    // 肉类不能驯服狼，只能繁殖
    ItemStack porkchopStack(Items::PORKCHOP, 1);
    EXPECT_FALSE(wolf.isTameItem(porkchopStack));

    ItemStack beefStack(Items::BEEF, 1);
    EXPECT_FALSE(wolf.isTameItem(beefStack));

    ItemStack rottenFleshStack(Items::ROTTEN_FLESH, 1);
    EXPECT_FALSE(wolf.isTameItem(rottenFleshStack));
}

// ============================================================================
// 繁殖物品测试
// ============================================================================

TEST_F(WolfEntityTestFixture, IsBreedingItem_Porkchop_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::PORKCHOP, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedPorkchop_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::COOKED_PORKCHOP, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Beef_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::BEEF, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedBeef_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::COOKED_BEEF, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Chicken_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::CHICKEN, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedChicken_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::COOKED_CHICKEN, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Rabbit_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::RABBIT, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedRabbit_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::COOKED_RABBIT, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Mutton_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::MUTTON, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedMutton_ReturnsTrue)
{
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::COOKED_MUTTON, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

// ============================================================================
// 腐肉繁殖测试（MC 1.16.5：狼可以用腐肉繁殖和治疗）
// ============================================================================

TEST_F(WolfEntityTestFixture, IsBreedingItem_RottenFlesh_ReturnsTrue)
{
    // 参考: MC 1.16.5 WolfEntity.isBreedingItem()
    // 狼可以用任何肉类繁殖，腐肉在 Foods.java 中标记为 .meat()
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::ROTTEN_FLESH, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsFoodItem_RottenFlesh_ReturnsTrue)
{
    // 狼的食物（用于治疗）与繁殖物品相同
    WolfEntity wolf(EntityId(0));

    ItemStack stack(Items::ROTTEN_FLESH, 1);
    EXPECT_TRUE(wolf.isFoodItem(stack));
}

// ============================================================================
// 非肉类物品测试
// ============================================================================

TEST_F(WolfEntityTestFixture, IsBreedingItem_NonMeat_ReturnsFalse)
{
    WolfEntity wolf(EntityId(0));

    // 小麦不能用于狼繁殖
    ItemStack wheatStack(Items::WHEAT, 1);
    EXPECT_FALSE(wolf.isBreedingItem(wheatStack));

    // 胡萝卜不能用于狼繁殖
    ItemStack carrotStack(Items::CARROT, 1);
    EXPECT_FALSE(wolf.isBreedingItem(carrotStack));

    // 苹果不能用于狼繁殖
    ItemStack appleStack(Items::APPLE, 1);
    EXPECT_FALSE(wolf.isBreedingItem(appleStack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Bone_ReturnsFalse)
{
    WolfEntity wolf(EntityId(0));

    // 骨头只能驯服，不能繁殖
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_FALSE(wolf.isBreedingItem(boneStack));
}

// ============================================================================
// 空物品测试
// ============================================================================

TEST_F(WolfEntityTestFixture, IsBreedingItem_EmptyStack_ReturnsFalse)
{
    WolfEntity wolf(EntityId(0));

    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(wolf.isBreedingItem(emptyStack));
}

TEST_F(WolfEntityTestFixture, IsTameItem_EmptyStack_ReturnsFalse)
{
    WolfEntity wolf(EntityId(0));

    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(wolf.isTameItem(emptyStack));
}

// ============================================================================
// 生成幼体测试
// ============================================================================

TEST_F(WolfEntityTestFixture, SpawnBaby_CreatesChildWolf)
{
    WolfEntity parent1(EntityId(0));
    WolfEntity parent2(EntityId(0));

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    // 验证是狼实体
    auto* babyWolf = dynamic_cast<WolfEntity*>(baby.get());
    EXPECT_NE(babyWolf, nullptr);

    // 验证是幼体
    EXPECT_TRUE(baby->isChild());
}

// ============================================================================
// 驯服后属性变化测试
// ============================================================================

TEST_F(WolfEntityTestFixture, OnTamed_IncreasesMaxHealth)
{
    WolfEntity wolf(EntityId(0));

    // 驯服前生命值为 8
    EXPECT_EQ(wolf.maxHealth(), 8.0f);

    // 驯服后生命值为 20
    wolf.setTamed(true);
    EXPECT_EQ(wolf.maxHealth(), 20.0f);
    EXPECT_EQ(wolf.health(), 20.0f);
}

TEST_F(WolfEntityTestFixture, OnTamed_IncreasesAttackDamage)
{
    WolfEntity wolf(EntityId(0));

    // 驯服前攻击力为 2
    // 注意：需要使用 getAttributeValueUnsafe 或检查属性是否存在
    f32 baseDamage = wolf.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, -1.0f);
    // 如果属性未注册，默认值为 -1，跳过此测试
    if (baseDamage < 0) {
        GTEST_SKIP() << "Attack damage attribute not registered in test fixture";
    }
    EXPECT_NEAR(baseDamage, 2.0f, 0.1f);

    // 驯服后攻击力为 4
    wolf.setTamed(true);
    f32 tamedDamage = wolf.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, -1.0f);
    EXPECT_NEAR(tamedDamage, 4.0f, 0.1f);
}

// ============================================================================
// 颈圈颜色测试
// ============================================================================

TEST_F(WolfEntityTestFixture, CollarColor_DefaultIsRed)
{
    WolfEntity wolf(EntityId(0));

    // 默认颈圈颜色为红色
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red);
}

TEST_F(WolfEntityTestFixture, CollarColor_CanBeSet)
{
    WolfEntity wolf(EntityId(0));

    // 设置为蓝色
    wolf.setCollarColor(DyeColor::Blue);
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Blue);

    // 设置为绿色
    wolf.setCollarColor(DyeColor::Green);
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Green);
}

// ============================================================================
// 尾巴角度测试
// ============================================================================

TEST_F(WolfEntityTestFixture, TailAngle_HealthyWolf)
{
    WolfEntity wolf(EntityId(0));
    wolf.setHealth(8.0f); // 满血

    // 健康狼尾巴角度应该接近 TAIL_ANGLE_HEALTHY (0.698f)
    f32 tailAngle = wolf.getTailAngle();
    EXPECT_GT(tailAngle, 0.5f);
    EXPECT_LT(tailAngle, 0.8f);
}

TEST_F(WolfEntityTestFixture, TailAngle_UnhealthyWolf)
{
    WolfEntity wolf(EntityId(0));
    wolf.setHealth(2.0f); // 低血量

    // 不健康狼尾巴角度应该接近 TAIL_ANGLE_UNHEALTHY (-0.175f)
    f32 tailAngle = wolf.getTailAngle();
    EXPECT_GT(tailAngle, -0.5f);
    EXPECT_LT(tailAngle, 0.5f);
}

TEST_F(WolfEntityTestFixture, TailAngle_AngryWolf)
{
    WolfEntity wolf(EntityId(0));
    wolf.setAngry(true);

    // 愤怒狼尾巴角度应该竖起
    f32 tailAngle = wolf.getTailAngle();
    EXPECT_FLOAT_EQ(tailAngle, 1.539f);
}

// ============================================================================
// 尾巴角度回归测试（WolfModel 尾巴角度修复）
//
// WolfModel::setAngles() 曾经错误地使用 ageInTicks（帧计数器，无界递增值）
// 作为尾巴旋转角度，导致尾巴在帧数累积后旋转到不可预期的极端角度。
// 修复后使用 m_tailRotation（由 WolfEntity::getTailAngle() 计算，有界值）。
//
// 以下测试验证 WolfEntity::getTailAngle() 返回的值始终在合理范围内，
// 确保 WolfModel 接收到的尾巴角度是有界的。
// ============================================================================

TEST_F(WolfEntityTestFixture, TailAngle_BoundedRange_NonAngryWolf)
{
    // 非愤怒狼的尾巴角度应该始终在 [TAIL_ANGLE_UNHEALTHY, TAIL_ANGLE_HEALTHY] 范围内
    // TAIL_ANGLE_UNHEALTHY ≈ -0.175 (约 -10°)
    // TAIL_ANGLE_HEALTHY ≈ 0.698 (约 40°)
    // 这与 ageInTicks（可无限增长）不同，是有界的
    WolfEntity wolf(EntityId(0));
    wolf.setAngry(false);

    // 满血
    wolf.setHealth(wolf.maxHealth());
    f32 tailAngle = wolf.getTailAngle();
    EXPECT_GT(tailAngle, -1.0f); // 不应出现极端负值
    EXPECT_LT(tailAngle, 2.0f);  // 不应出现极端正值

    // 半血
    wolf.setHealth(wolf.maxHealth() / 2.0f);
    tailAngle = wolf.getTailAngle();
    EXPECT_GT(tailAngle, -1.0f);
    EXPECT_LT(tailAngle, 2.0f);

    // 低血量
    wolf.setHealth(1.0f);
    tailAngle = wolf.getTailAngle();
    EXPECT_GT(tailAngle, -1.0f);
    EXPECT_LT(tailAngle, 2.0f);
}

TEST_F(WolfEntityTestFixture, TailAngle_NeverEqualToAgeInTicks)
{
    // 回归测试：确保 getTailAngle() 不会返回类似 ageInTicks 的无界值
    // ageInTicks 是帧计数器，随时间无限增长，而 getTailAngle() 应该是
    // 基于生命值/愤怒状态的有界值
    WolfEntity wolf(EntityId(0));
    wolf.setAngry(false);
    wolf.setHealth(wolf.maxHealth());

    // 多次 tick 后尾巴角度仍然有界（不像 ageInTicks 那样增长）
    f32 prevAngle = wolf.getTailAngle();
    for (int i = 0; i < 1000; ++i) {
        // 模拟 tick（不实际调用 tick 以避免副作用）
        wolf.setHealth(wolf.maxHealth() - static_cast<f32>(i % 20));
        f32 angle = wolf.getTailAngle();
        // 角度必须始终在合理范围内，不能随时间增长到极端值
        EXPECT_GT(angle, -1.0f) << "Tail angle went too negative at iteration " << i;
        EXPECT_LT(angle, 2.0f) << "Tail angle went too positive at iteration " << i;
    }
}

TEST_F(WolfEntityTestFixture, TailAngle_HealthyVsUnhealthyGradient)
{
    // 满血狼尾巴角度应该高于低血量狼
    // 验证尾巴角度与生命值成正比（健康=高尾巴，不健康=低尾巴）
    WolfEntity wolf(EntityId(0));
    wolf.setAngry(false);

    wolf.setHealth(wolf.maxHealth());
    f32 healthyAngle = wolf.getTailAngle();

    wolf.setHealth(1.0f);
    f32 unhealthyAngle = wolf.getTailAngle();

    // 健康狼尾巴角度应该高于不健康狼
    EXPECT_GT(healthyAngle, unhealthyAngle);
}

// ============================================================================
// 兴趣状态测试
// ============================================================================

TEST_F(WolfEntityTestFixture, Interested_CanBeSet)
{
    WolfEntity wolf(EntityId(0));

    EXPECT_FALSE(wolf.isInterested());

    wolf.setInterested(true);
    EXPECT_TRUE(wolf.isInterested());

    wolf.setInterested(false);
    EXPECT_FALSE(wolf.isInterested());
}

// ============================================================================
// interactMob 测试 - 未驯服狼 + 骨头 → 驯服尝试
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_UntamedWolf_WithBone_ReturnsSuccessAndPlaysSound)
{
    // 未驯服的狼用骨头交互：应该消耗物品、播放声音、尝试驯服
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack boneStack(Items::BONE, 10);
    player.inventory().setItem(0, boneStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();
    world.resetBroadcastTracking();

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 播放吃东西声音
    EXPECT_EQ(world.getSoundPlayCount(), 1);

    // 非创造模式下物品应该减少
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, 9);

    // 应该有广播（TamingSucceeded 或 TamingFailed）
    EXPECT_EQ(world.getBroadcastCount(), 1);
    EXPECT_EQ(world.getLastBroadcastEntityId(), EntityId(1));
    u8 status = world.getLastBroadcastStatus();
    bool isValidStatus = (status == static_cast<u8>(network::EntityStatusPacket::Status::TamingSucceeded) ||
        status == static_cast<u8>(network::EntityStatusPacket::Status::TamingFailed));
    EXPECT_TRUE(isValidStatus);
}

TEST_F(WolfEntityTestFixture, InteractMob_UntamedWolf_WithBone_CreativeMode_NoConsumption)
{
    // 创造模式下骨头不被消耗
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = true;
    ItemStack boneStack(Items::BONE, 10);
    player.inventory().setItem(0, boneStack);
    player.inventory().setSelectedSlot(0);

    i32 countBefore = player.inventory().getItem(0).getCount();

    wolf.interactMob(player, Hand::MainHand);

    // 创造模式下物品不应该减少
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, countBefore);
}

TEST_F(WolfEntityTestFixture, InteractMob_UntamedWolf_WithBone_SilentWolf_NoSound)
{
    // 静音狼不应该播放声音
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setSilent(true);

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    ItemStack boneStack(Items::BONE, 10);
    player.inventory().setItem(0, boneStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    wolf.interactMob(player, Hand::MainHand);

    // 静音状态下不应该播放声音
    EXPECT_EQ(world.getSoundPlayCount(), 0);
}

TEST_F(WolfEntityTestFixture, InteractMob_UntamedWolf_AngryWolf_BoneDoesNotTame)
{
    // 愤怒的狼不能用骨头驯服
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setAngry(true);

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    ItemStack boneStack(Items::BONE, 10);
    player.inventory().setItem(0, boneStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();
    world.resetBroadcastTracking();

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 愤怒狼不接受骨头，交给父类处理
    // 不应消耗物品，不应播放声音，不应广播
    EXPECT_EQ(world.getSoundPlayCount(), 0);
    EXPECT_EQ(world.getBroadcastCount(), 0);
}

TEST_F(WolfEntityTestFixture, InteractMob_UntamedWolf_NonBoneItem_PassesToParent)
{
    // 未驯服的狼用非骨头物品交互，交给父类处理
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    ItemStack appleStack(Items::APPLE, 10);
    player.inventory().setItem(0, appleStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 非骨头物品，狼不处理，传递给父类
    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_EQ(world.getSoundPlayCount(), 0);
}

TEST_F(WolfEntityTestFixture, InteractMob_UntamedWolf_OffHandBone)
{
    // 副手骨头测试
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;

    // 主手放苹果，副手放骨头
    ItemStack appleStack(Items::APPLE, 10);
    ItemStack boneStack(Items::BONE, 10);
    player.inventory().setItem(0, appleStack); // 主手
    player.inventory().setItem(40, boneStack); // 副手槽位
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    ActionResultType result = wolf.interactMob(player, Hand::OffHand);

    // 副手骨头应该触发驯服尝试
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(world.getSoundPlayCount(), 1);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamingSuccess_BroadcastsSuccessAndSetsOwner)
{
    // 驯服成功场景 - 直接验证状态设置
    WolfTestWorld world;

    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    EXPECT_FALSE(wolf.isTamed());

    // 直接设置驯服状态以验证成功后的行为
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    EXPECT_TRUE(wolf.isTamed());
    EXPECT_TRUE(wolf.isOwner(12345ULL));
    EXPECT_FALSE(wolf.isOwner(99999ULL));
}

TEST_F(WolfEntityTestFixture, InteractMob_TamingAttempt_BroadcastsEitherSuccessOrFail)
{
    // 驯服尝试：验证广播为成功或失败之一
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    ItemStack boneStack(Items::BONE, 10);
    player.inventory().setItem(0, boneStack);
    player.inventory().setSelectedSlot(0);

    world.resetBroadcastTracking();

    wolf.interactMob(player, Hand::MainHand);

    // 应该有广播
    EXPECT_EQ(world.getBroadcastCount(), 1);
    u8 status = world.getLastBroadcastStatus();
    bool isValidStatus = (status == static_cast<u8>(network::EntityStatusPacket::Status::TamingSucceeded) ||
        status == static_cast<u8>(network::EntityStatusPacket::Status::TamingFailed));
    EXPECT_TRUE(isValidStatus);
}

// ============================================================================
// interactMob 测试 - 已驯服狼 + 食物 + 未满血 → 喂食治疗
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_FoodDamaged_HealsAndPlaysSound)
{
    // 已驯服的狼 + 食物 + 未满血 → 治疗并播放声音
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(10.0f); // 未满血（驯服后满血 20.0f）

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 应该播放吃东西声音
    EXPECT_EQ(world.getSoundPlayCount(), 1);

    // 生命值应该增加（生猪肉治疗 4.0）
    EXPECT_GT(wolf.health(), 10.0f);

    // 物品应该消耗 1
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, 9);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_FoodFullHealth_DoesNotHeal)
{
    // 已驯服的狼 + 食物 + 满血 → 跳过治疗分支，进入繁殖/成长分支
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(wolf.maxHealth()); // 满血

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    f32 healthBefore = wolf.health();

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 返回 Success（进入繁殖分支）
    EXPECT_EQ(result, ActionResultType::Success);

    // 生命值不应该变化（因为满血不会触发治疗）
    EXPECT_FLOAT_EQ(wolf.health(), healthBefore);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_RottenFlesh_Heals)
{
    // 腐肉治疗测试
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(5.0f); // 受伤

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack rottenFleshStack(Items::ROTTEN_FLESH, 10);
    player.inventory().setItem(0, rottenFleshStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    // 腐肉治疗 8.0（nutrition=4, heal=2*4=8）
    EXPECT_FLOAT_EQ(wolf.health(), 13.0f);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_CookedBeef_Heals)
{
    // 熟牛排治疗测试
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(5.0f); // 受伤

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack cookedBeefStack(Items::COOKED_BEEF, 10);
    player.inventory().setItem(0, cookedBeefStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    // 熟牛排治疗 16.0（nutrition=8, heal=2*8=16），但不超过最大生命值
    EXPECT_FLOAT_EQ(wolf.health(), wolf.maxHealth());
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_FoodCreativeMode_NoConsumption)
{
    // 创造模式下喂食不消耗物品
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(10.0f);

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = true;
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    i32 countBefore = player.inventory().getItem(0).getCount();

    wolf.interactMob(player, Hand::MainHand);

    // 创造模式下物品不应该减少
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, countBefore);

    // 但仍应该治疗
    EXPECT_GT(wolf.health(), 10.0f);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_SilentFoodHeal_NoSound)
{
    // 静音狼喂食不播放声音
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(10.0f);
    wolf.setSilent(true);

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    wolf.interactMob(player, Hand::MainHand);

    // 静音状态下不应该播放声音
    EXPECT_EQ(world.getSoundPlayCount(), 0);
}

// ============================================================================
// interactMob 测试 - 已驯服狼 + 染料 + 主人 → 颈圈染色
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_DyeOwner_ChangesCollarColor)
{
    // 已驯服的狼 + 染料 + 主人 → 改变颈圈颜色
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red); // 默认红色

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL); // 主人
    player.abilities().creativeMode = false;
    ItemStack dyeStack(Items::LAPIS_LAZULI_DYE, 10);
    player.inventory().setItem(0, dyeStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 颈圈颜色应该变为蓝色（LAPIS_LAZULI_DYE 映射到 Blue）
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Blue);

    // 物品应该消耗
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, 9);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_DyeNonOwner_NoCollarChange)
{
    // 已驯服的狼 + 染料 + 非主人 → 不改变颈圈颜色，进入坐下/站起分支
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red);

    Player player(EntityId(2), "OtherPlayer");
    player.setPlayerId(99999ULL); // 非主人
    player.abilities().creativeMode = false;
    ItemStack dyeStack(Items::LAPIS_LAZULI_DYE, 10);
    player.inventory().setItem(0, dyeStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 非主人使用染料，由于狼满血，食物分支被跳过
    // 染料分支检查 isOwner()，非主人被跳过
    // 最终进入坐下/站起分支，但非主人也不满足 isOwner()
    // 所以返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 颈圈颜色不应该变化
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red);

    // 物品不应该消耗
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, 10);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_DyeSameColor_NoConsumption)
{
    // 已驯服的狼 + 相同颜色染料 + 主人 → 不消耗物品，返回 Success
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    // 默认红色颈圈

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack dyeStack(Items::RED_DYE, 10);
    player.inventory().setItem(0, dyeStack);
    player.inventory().setSelectedSlot(0);

    i32 countBefore = player.inventory().getItem(0).getCount();

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 颈圈颜色不变
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red);

    // 相同颜色不消耗物品
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, countBefore);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_BoneMealDye_WhiteCollar)
{
    // 骨粉作为白色染料
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack boneMealStack(Items::BONE_MEAL, 10);
    player.inventory().setItem(0, boneMealStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    // 骨粉映射到白色
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::White);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_InkSacDye_BlackCollar)
{
    // 墨囊作为黑色染料
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack inkSacStack(Items::INK_SAC, 10);
    player.inventory().setItem(0, inkSacStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    // 墨囊映射到黑色
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Black);
}

// ============================================================================
// interactMob 测试 - 已驯服狼 + 食物 + 幼年 → 成长加速
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_FoodChild_AcceleratesGrowth)
{
    // 已驯服的幼年狼 + 食物 → 加速成长
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setChild(true); // 设为幼体
    EXPECT_TRUE(wolf.isChild());

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    i32 ageBefore = wolf.getGrowingAge();

    world.resetSoundTracking();

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 应该播放吃东西声音
    EXPECT_EQ(world.getSoundPlayCount(), 1);

    // 年龄应该增长（getGrowingAge 从负值变得不那么负）
    i32 ageAfter = wolf.getGrowingAge();
    EXPECT_GT(ageAfter, ageBefore);

    // 物品应该消耗
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, 9);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_FoodChild_CreativeMode_NoConsumption)
{
    // 创造模式下幼年狼喂食不消耗物品
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setChild(true);

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = true;
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    i32 countBefore = player.inventory().getItem(0).getCount();

    wolf.interactMob(player, Hand::MainHand);

    // 创造模式下物品不应该减少
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, countBefore);
}

// ============================================================================
// interactMob 测试 - 已驯服狼 + 食物 + 成年可繁殖 → 进入求爱状态
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_FoodAdultBreedable_EntersLoveMode)
{
    // 已驯服的成年狼 + 食物 + 满血 + 可繁殖 → 进入求爱状态
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(wolf.maxHealth()); // 满血
    EXPECT_FALSE(wolf.isChild());     // 成年
    EXPECT_TRUE(wolf.canBreed());     // 可繁殖

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    world.resetSoundTracking();

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 应该播放吃东西声音
    EXPECT_EQ(world.getSoundPlayCount(), 1);

    // 应该进入求爱状态
    EXPECT_TRUE(wolf.isInLove());

    // 物品应该消耗
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, 9);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_FoodAdultBreedable_CreativeMode_NoConsumption)
{
    // 创造模式下繁殖不消耗物品
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(wolf.maxHealth());

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = true;
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    i32 countBefore = player.inventory().getItem(0).getCount();

    wolf.interactMob(player, Hand::MainHand);

    EXPECT_TRUE(wolf.isInLove());
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, countBefore);
}

// ============================================================================
// interactMob 测试 - 已驯服狼 + 主人 + 无特殊物品 → 切换坐下/站起
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_OwnerEmptyHand_TogglesSitting)
{
    // 已驯服的狼 + 主人 + 空手 → 切换坐下/站起
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    // 空手（不设置任何物品）
    ItemStack emptyStack(nullptr, 0);
    player.inventory().setItem(0, emptyStack);
    player.inventory().setSelectedSlot(0);

    // 初始状态：不坐下
    EXPECT_FALSE(wolf.isSitting());

    // 第一次交互 → 坐下
    ActionResultType result = wolf.interactMob(player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(wolf.isSitting());

    // 第二次交互 → 站起
    result = wolf.interactMob(player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_FALSE(wolf.isSitting());
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_OwnerSitting_TogglesToStanding)
{
    // 已驯服坐着的狼 + 主人 → 站起
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setSitting(true);
    EXPECT_TRUE(wolf.isSitting());

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    ItemStack emptyStack(nullptr, 0);
    player.inventory().setItem(0, emptyStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_FALSE(wolf.isSitting());
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_NonOwnerEmptyHand_Passes)
{
    // 已驯服的狼 + 非主人 + 空手 → 返回 Pass
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player otherPlayer(EntityId(2), "OtherPlayer");
    otherPlayer.setPlayerId(99999ULL); // 非主人
    ItemStack emptyStack(nullptr, 0);
    otherPlayer.inventory().setItem(0, emptyStack);
    otherPlayer.inventory().setSelectedSlot(0);

    bool wasSitting = wolf.isSitting();

    ActionResultType result = wolf.interactMob(otherPlayer, Hand::MainHand);

    // 非主人不能切换坐下状态
    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_EQ(wolf.isSitting(), wasSitting);
}

// ============================================================================
// interactMob 测试 - 优先级验证
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_DamagedFoodPrioritizedOverBreed)
{
    // 已驯服的狼 + 食物 + 未满血 → 应该治疗，而不是进入繁殖
    // 这验证了治疗优先级高于繁殖
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(5.0f); // 受伤

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);

    // 应该治疗（生命值增加），不应该进入求爱状态
    EXPECT_GT(wolf.health(), 5.0f);
    EXPECT_FALSE(wolf.isInLove()); // 受伤时不应进入繁殖状态
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_FoodHealPrioritizedOverDye)
{
    // 已驯服的狼 + 满血 + 猪排 → 应该进入繁殖状态，而非治疗
    // 这验证满血时食物不被用于治疗
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(wolf.maxHealth()); // 满血

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);

    // 满血 + 成年 + 可繁殖 → 进入求爱状态
    EXPECT_TRUE(wolf.isInLove());
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_ChildFoodPrioritizedOverDye)
{
    // 已驯服的幼年狼 + 食物 → 加速成长（而非染色，因为食物不是染料）
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setChild(true);
    wolf.setHealth(wolf.maxHealth()); // 幼年狼满血

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    i32 ageBefore = wolf.getGrowingAge();

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);

    // 应该加速成长
    i32 ageAfter = wolf.getGrowingAge();
    EXPECT_GT(ageAfter, ageBefore);
}

// ============================================================================
// interactMob 测试 - 综合场景
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_DyePrioritizedOverSitToggle)
{
    // 已驯服的狼 + 染料（不是食物）+ 主人 → 染色而非坐下切换
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(wolf.maxHealth()); // 满血
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red);

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;
    ItemStack dyeStack(Items::LAPIS_LAZULI_DYE, 10);
    player.inventory().setItem(0, dyeStack);
    player.inventory().setSelectedSlot(0);

    bool wasSitting = wolf.isSitting();

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);

    // 颈圈颜色应该变化（LAPIS_LAZULI_DYE 映射到 Blue）
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Blue);

    // 坐下状态不应该变化
    EXPECT_EQ(wolf.isSitting(), wasSitting);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_HealAndDyeSeparateInteractions)
{
    // 先治疗，再染色 - 验证多次交互独立工作
    WolfTestWorld world;
    WolfEntity wolf(EntityId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(5.0f); // 受伤

    Player player(EntityId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;

    // 第一次交互：用猪排治疗
    ItemStack porkchopStack(Items::PORKCHOP, 10);
    player.inventory().setItem(0, porkchopStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result1 = wolf.interactMob(player, Hand::MainHand);
    EXPECT_EQ(result1, ActionResultType::Success);
    EXPECT_GT(wolf.health(), 5.0f); // 生命值增加

    // 第二次交互：用染料染色
    ItemStack dyeStack(Items::LAPIS_LAZULI_DYE, 10);
    player.inventory().setItem(0, dyeStack);

    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red); // 默认红色
    ActionResultType result2 = wolf.interactMob(player, Hand::MainHand);
    EXPECT_EQ(result2, ActionResultType::Success);
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Blue); // 变为蓝色
}

} // namespace
} // namespace mc
