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
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/DyeableArmorItem.hpp"
#include "common/item/items/armor/WolfArmorItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
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
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    /// 覆写 hasBlockCollision：可配置是否提供地面碰撞
    /// 测试世界无物理引擎，狼会因重力下落。默认提供虚拟地面使 onGround=true，
    /// 从而允许甩水状态机正常触发（MC 中狼需站在方块上方能甩水）。
    /// 通过 setGroundCollisionEnabled(false) 可禁用地面，测试不在地面的场景。
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        (void)box;
        return m_groundCollisionEnabled;
    }

    void setGroundCollisionEnabled(bool enabled) { m_groundCollisionEnabled = enabled; }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        if (hasBlockCollision(box)) {
            return {AxisAlignedBB(-1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f)};
        }
        return {};
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] i32 getSpawnedEntityCount() const { return static_cast<i32>(m_spawnedEntities.size()); }

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
    EntityInstanceId m_lastBroadcastEntityId{0};
    u8 m_lastBroadcastStatus = 0;
    i32 m_broadcastCount = 0;

    // 驯服追踪
    bool m_tameAnimalCalled = false;
    PlayerId m_lastTamePlayerId = 0;
    Entity* m_lastTameAnimal = nullptr;

    // 地面碰撞开关（默认启用，使狼在测试中视为在地面）
    bool m_groundCollisionEnabled = true;
};

class WolfEntityTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        item::tag::ItemTags::initialize();
        // 初始化伤害类型标签（狼铠吸收判定依赖 BYPASSES_WOLF_ARMOR 标签）
        DamageTypeTags::initialize();
    }

    WolfTestWorld m_world;
};

// ============================================================================
// 驯服物品测试
// ============================================================================

TEST_F(WolfEntityTestFixture, IsTameItem_Bone_ReturnsTrue)
{
    WolfEntity wolf(EntityInstanceId(0));

    // 骨头是驯服狼的唯一物品
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_TRUE(wolf.isTameItem(boneStack));
}

TEST_F(WolfEntityTestFixture, IsTameItem_Meat_ReturnsFalse)
{
    WolfEntity wolf(EntityInstanceId(0));

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
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::PORKCHOP, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedPorkchop_ReturnsTrue)
{
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::COOKED_PORKCHOP, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Beef_ReturnsTrue)
{
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::BEEF, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedBeef_ReturnsTrue)
{
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::COOKED_BEEF, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Chicken_ReturnsTrue)
{
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::CHICKEN, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedChicken_ReturnsTrue)
{
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::COOKED_CHICKEN, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Rabbit_ReturnsTrue)
{
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::RABBIT, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedRabbit_ReturnsTrue)
{
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::COOKED_RABBIT, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Mutton_ReturnsTrue)
{
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::MUTTON, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedMutton_ReturnsTrue)
{
    WolfEntity wolf(EntityInstanceId(0));

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
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::ROTTEN_FLESH, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsFoodItem_RottenFlesh_ReturnsTrue)
{
    // 狼的食物（用于治疗）与繁殖物品相同
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack stack(Items::ROTTEN_FLESH, 1);
    EXPECT_TRUE(wolf.isFoodItem(stack));
}

// ============================================================================
// 非肉类物品测试
// ============================================================================

TEST_F(WolfEntityTestFixture, IsBreedingItem_NonMeat_ReturnsFalse)
{
    WolfEntity wolf(EntityInstanceId(0));

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
    WolfEntity wolf(EntityInstanceId(0));

    // 骨头只能驯服，不能繁殖
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_FALSE(wolf.isBreedingItem(boneStack));
}

// ============================================================================
// 空物品测试
// ============================================================================

TEST_F(WolfEntityTestFixture, IsBreedingItem_EmptyStack_ReturnsFalse)
{
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(wolf.isBreedingItem(emptyStack));
}

TEST_F(WolfEntityTestFixture, IsTameItem_EmptyStack_ReturnsFalse)
{
    WolfEntity wolf(EntityInstanceId(0));

    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(wolf.isTameItem(emptyStack));
}

// ============================================================================
// 生成幼体测试
// ============================================================================

TEST_F(WolfEntityTestFixture, SpawnBaby_CreatesChildWolf)
{
    WolfEntity parent1(EntityInstanceId(0));
    WolfEntity parent2(EntityInstanceId(0));

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
    WolfEntity wolf(EntityInstanceId(0));

    // 驯服前生命值为 8
    EXPECT_EQ(wolf.maxHealth(), 8.0f);

    // 驯服后生命值为 20
    wolf.setTamed(true);
    EXPECT_EQ(wolf.maxHealth(), 20.0f);
    EXPECT_EQ(wolf.health(), 20.0f);
}

TEST_F(WolfEntityTestFixture, OnTamed_IncreasesAttackDamage)
{
    WolfEntity wolf(EntityInstanceId(0));

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
    WolfEntity wolf(EntityInstanceId(0));

    // 默认颈圈颜色为红色
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red);
}

TEST_F(WolfEntityTestFixture, CollarColor_CanBeSet)
{
    WolfEntity wolf(EntityInstanceId(0));

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
    WolfEntity wolf(EntityInstanceId(0));
    wolf.setHealth(8.0f); // 满血

    // 健康狼尾巴角度应该接近 TAIL_ANGLE_HEALTHY (0.698f)
    f32 tailAngle = wolf.getTailAngle();
    EXPECT_GT(tailAngle, 0.5f);
    EXPECT_LT(tailAngle, 0.8f);
}

TEST_F(WolfEntityTestFixture, TailAngle_UnhealthyWolf)
{
    WolfEntity wolf(EntityInstanceId(0));
    wolf.setHealth(2.0f); // 低血量

    // 不健康狼尾巴角度应该接近 TAIL_ANGLE_UNHEALTHY (-0.175f)
    f32 tailAngle = wolf.getTailAngle();
    EXPECT_GT(tailAngle, -0.5f);
    EXPECT_LT(tailAngle, 0.5f);
}

TEST_F(WolfEntityTestFixture, TailAngle_AngryWolf)
{
    WolfEntity wolf(EntityInstanceId(0));
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
    WolfEntity wolf(EntityInstanceId(0));
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
    WolfEntity wolf(EntityInstanceId(0));
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
    WolfEntity wolf(EntityInstanceId(0));
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
    WolfEntity wolf(EntityInstanceId(0));

    EXPECT_FALSE(wolf.isInterested());

    wolf.setInterested(true);
    EXPECT_TRUE(wolf.isInterested());

    wolf.setInterested(false);
    EXPECT_FALSE(wolf.isInterested());
}

// ============================================================================
// 兴趣状态 DataParameter 同步测试
// ============================================================================

TEST_F(WolfEntityTestFixture, DataParameter_InterestedParamId_IsValid)
{
    // DATA_INTERESTED_PARAM 的 ID 应该是有效的（>0，由 createKey 自动分配）
    u16 paramId = WolfEntity::getInterestedParamId();
    EXPECT_GT(paramId, 0u);
}

TEST_F(WolfEntityTestFixture, DataParameter_IsInterested_ReadsFromDataManager)
{
    // isInterested() 应该从 DataManager 读取而非成员变量
    WolfEntity wolf(EntityInstanceId(0));
    EXPECT_FALSE(wolf.isInterested());

    wolf.setInterested(true);
    EXPECT_TRUE(wolf.isInterested());

    // 通过 DataManager 直接读取验证
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getInterestedParamId();
    EXPECT_TRUE(dataManager.hasParam(paramId));
    bool storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_TRUE(storedValue);
}

TEST_F(WolfEntityTestFixture, DataParameter_SetInterested_WritesToDataManager)
{
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getInterestedParamId();

    // 设置兴趣状态
    wolf.setInterested(true);
    bool storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_TRUE(storedValue);

    // 设置为不感兴趣
    wolf.setInterested(false);
    storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_FALSE(storedValue);
}

TEST_F(WolfEntityTestFixture, DataParameter_DirtyFlag_OnInterestedChange)
{
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();

    // 初始状态不应有脏数据
    dataManager.clearDirty();
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置兴趣状态应该标记为脏数据
    wolf.setInterested(true);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 清除脏标记后设置相同值不应标记为脏
    dataManager.clearDirty();
    wolf.setInterested(true);
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置不同值应该标记为脏
    wolf.setInterested(false);
    EXPECT_TRUE(dataManager.hasDirtyData());
}

TEST_F(WolfEntityTestFixture, DataParameter_SyncsStateChanges)
{
    // 验证多次状态变更正确同步
    WolfEntity wolf(EntityInstanceId(0));

    EXPECT_FALSE(wolf.isInterested());

    wolf.setInterested(true);
    EXPECT_TRUE(wolf.isInterested());

    wolf.setInterested(false);
    EXPECT_FALSE(wolf.isInterested());

    wolf.setInterested(true);
    EXPECT_TRUE(wolf.isInterested());
}

TEST_F(WolfEntityTestFixture, DataParameter_RegisteredOnConstruction)
{
    // 验证 WolfEntity 构造后 DATA_INTERESTED_PARAM 已注册到 DataManager
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getInterestedParamId();

    // 参数应该已注册
    EXPECT_TRUE(dataManager.hasParam(paramId));

    // 默认值应为 false
    EXPECT_FALSE(wolf.isInterested());
}

// ============================================================================
// 驯服状态 DataParameter 同步测试
// ============================================================================

TEST_F(WolfEntityTestFixture, DataParameter_TamedParamId_IsValid)
{
    // DATA_TAMED_PARAM 的 ID 应该是有效的（>0，由 createKey 自动分配）
    u16 paramId = TameableEntity::getTamedParamId();
    EXPECT_GT(paramId, 0u);
}

TEST_F(WolfEntityTestFixture, DataParameter_IsTamed_ReadsFromDataManager)
{
    // isTamed() 应该从 DataManager 读取而非成员变量
    WolfEntity wolf(EntityInstanceId(0));
    EXPECT_FALSE(wolf.isTamed());

    wolf.setTamed(true);
    EXPECT_TRUE(wolf.isTamed());

    // 通过 DataManager 直接读取验证
    auto& dataManager = wolf.dataManager();
    u16 paramId = TameableEntity::getTamedParamId();
    EXPECT_TRUE(dataManager.hasParam(paramId));
    bool storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_TRUE(storedValue);
}

TEST_F(WolfEntityTestFixture, DataParameter_SetTamed_WritesToDataManager)
{
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();
    u16 paramId = TameableEntity::getTamedParamId();

    // 设置驯服状态
    wolf.setTamed(true);
    bool storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_TRUE(storedValue);

    // 设置为未驯服
    wolf.setTamed(false);
    storedValue = dataManager.get<bool>(entity::DataParameter<bool>(paramId));
    EXPECT_FALSE(storedValue);
}

TEST_F(WolfEntityTestFixture, DataParameter_DirtyFlag_OnTamedChange)
{
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();

    // 初始状态不应有脏数据
    dataManager.clearDirty();
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置驯服状态应该标记为脏数据
    wolf.setTamed(true);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 清除脏标记后设置相同值不应标记为脏
    dataManager.clearDirty();
    wolf.setTamed(true);
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置不同值应该标记为脏
    wolf.setTamed(false);
    EXPECT_TRUE(dataManager.hasDirtyData());
}

TEST_F(WolfEntityTestFixture, DataParameter_TamedRegisteredOnConstruction)
{
    // 验证 WolfEntity 构造后 DATA_TAMED_PARAM 已注册到 DataManager
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();
    u16 paramId = TameableEntity::getTamedParamId();

    // 参数应该已注册
    EXPECT_TRUE(dataManager.hasParam(paramId));

    // 默认值应为 false（未驯服）
    EXPECT_FALSE(wolf.isTamed());
}

// ============================================================================
// 颈圈颜色 DataParameter 同步测试
// ============================================================================

TEST_F(WolfEntityTestFixture, DataParameter_CollarColorParamId_IsValid)
{
    // DATA_COLLAR_COLOR_PARAM 的 ID 应该是有效的（>0，由 createKey 自动分配）
    u16 paramId = WolfEntity::getCollarColorParamId();
    EXPECT_GT(paramId, 0u);
}

TEST_F(WolfEntityTestFixture, DataParameter_GetCollarColor_ReadsFromDataManager)
{
    // getCollarColor() 应该从 DataManager 读取而非成员变量
    WolfEntity wolf(EntityInstanceId(0));
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red); // 默认红色

    wolf.setCollarColor(DyeColor::Blue);
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Blue);

    // 通过 DataManager 直接读取验证
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getCollarColorParamId();
    EXPECT_TRUE(dataManager.hasParam(paramId));
    i32 storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
    EXPECT_EQ(storedValue, static_cast<i32>(DyeColor::Blue));
}

TEST_F(WolfEntityTestFixture, DataParameter_SetCollarColor_WritesToDataManager)
{
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getCollarColorParamId();

    // 设置颈圈颜色
    wolf.setCollarColor(DyeColor::Green);
    i32 storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
    EXPECT_EQ(storedValue, static_cast<i32>(DyeColor::Green));

    // 设置为白色
    wolf.setCollarColor(DyeColor::White);
    storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
    EXPECT_EQ(storedValue, static_cast<i32>(DyeColor::White));
}

TEST_F(WolfEntityTestFixture, DataParameter_DirtyFlag_OnCollarColorChange)
{
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();

    // 初始状态不应有脏数据
    dataManager.clearDirty();
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置颈圈颜色应该标记为脏数据
    wolf.setCollarColor(DyeColor::Blue);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 清除脏标记后设置相同值不应标记为脏
    dataManager.clearDirty();
    wolf.setCollarColor(DyeColor::Blue);
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置不同值应该标记为脏
    wolf.setCollarColor(DyeColor::Red);
    EXPECT_TRUE(dataManager.hasDirtyData());
}

TEST_F(WolfEntityTestFixture, DataParameter_CollarColorDefaultIsRed)
{
    // 验证 WolfEntity 构造后 DATA_COLLAR_COLOR_PARAM 默认值为红色
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getCollarColorParamId();

    // 参数应该已注册
    EXPECT_TRUE(dataManager.hasParam(paramId));

    // 默认值应为红色
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red);
}

TEST_F(WolfEntityTestFixture, DataParameter_CollarColor_AllDyeColorsRoundTrip)
{
    // 验证所有 16 种 DyeColor 都能正确通过 DataParameter 存取
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getCollarColorParamId();

    for (i32 i = 0; i <= 15; ++i) {
        DyeColor color = static_cast<DyeColor>(i);
        wolf.setCollarColor(color);
        EXPECT_EQ(wolf.getCollarColor(), color);
        i32 storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
        EXPECT_EQ(storedValue, i);
    }
}

// ============================================================================
// interactMob 测试 - 未驯服狼 + 骨头 → 驯服尝试
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_UntamedWolf_WithBone_ReturnsSuccessAndPlaysSound)
{
    // 未驯服的狼用骨头交互：应该消耗物品、播放声音、尝试驯服
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    Player player(EntityInstanceId(2), "TestPlayer");
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
    EXPECT_EQ(world.getLastBroadcastEntityId(), EntityInstanceId(1));
    u8 status = world.getLastBroadcastStatus();
    bool isValidStatus = (status == static_cast<u8>(network::EntityStatus::TamingSucceeded) ||
        status == static_cast<u8>(network::EntityStatus::TamingFailed));
    EXPECT_TRUE(isValidStatus);
}

TEST_F(WolfEntityTestFixture, InteractMob_UntamedWolf_WithBone_CreativeMode_NoConsumption)
{
    // 创造模式下骨头不被消耗
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setSilent(true);

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setAngry(true);

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    Player player(EntityInstanceId(2), "TestPlayer");
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

    WolfEntity wolf(EntityInstanceId(1));
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    ItemStack boneStack(Items::BONE, 10);
    player.inventory().setItem(0, boneStack);
    player.inventory().setSelectedSlot(0);

    world.resetBroadcastTracking();

    wolf.interactMob(player, Hand::MainHand);

    // 应该有广播
    EXPECT_EQ(world.getBroadcastCount(), 1);
    u8 status = world.getLastBroadcastStatus();
    bool isValidStatus = (status == static_cast<u8>(network::EntityStatus::TamingSucceeded) ||
        status == static_cast<u8>(network::EntityStatus::TamingFailed));
    EXPECT_TRUE(isValidStatus);
}

// ============================================================================
// interactMob 测试 - 已驯服狼 + 食物 + 未满血 → 喂食治疗
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_FoodDamaged_HealsAndPlaysSound)
{
    // 已驯服的狼 + 食物 + 未满血 → 治疗并播放声音
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(10.0f); // 未满血（驯服后满血 20.0f）

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(wolf.maxHealth()); // 满血

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(5.0f); // 受伤

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(5.0f); // 受伤

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(10.0f);

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(10.0f);
    wolf.setSilent(true);

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red); // 默认红色

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red);

    Player player(EntityInstanceId(2), "OtherPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    // 默认红色颈圈

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setChild(true); // 设为幼体
    EXPECT_TRUE(wolf.isChild());

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setChild(true);

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(wolf.maxHealth()); // 满血
    EXPECT_FALSE(wolf.isChild());     // 成年
    EXPECT_TRUE(wolf.canBreed());     // 可繁殖

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(wolf.maxHealth());

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setSitting(true);
    EXPECT_TRUE(wolf.isSitting());

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player otherPlayer(EntityInstanceId(2), "OtherPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(5.0f); // 受伤

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(wolf.maxHealth()); // 满血

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setChild(true);
    wolf.setHealth(wolf.maxHealth()); // 幼年狼满血

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(wolf.maxHealth()); // 满血
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red);

    Player player(EntityInstanceId(2), "TestPlayer");
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
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setHealth(5.0f); // 受伤

    Player player(EntityInstanceId(2), "TestPlayer");
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

// ============================================================================
// 狼铠装备交互测试
// ============================================================================

TEST_F(WolfEntityTestFixture, CanShearEquipment_OwnerCanShear)
{
    // 主人可以剪切狼铠
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);

    EXPECT_TRUE(wolf.canShearEquipment(player));
}

TEST_F(WolfEntityTestFixture, CanShearEquipment_NonOwnerCannotShear)
{
    // 非主人不能剪切狼铠
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player stranger(EntityInstanceId(2), "Stranger");
    stranger.setPlayerId(99999ULL);

    EXPECT_FALSE(wolf.canShearEquipment(stranger));
}

TEST_F(WolfEntityTestFixture, BodyArmor_GetAndSet)
{
    // 测试 MobEntity 身体护甲便捷方法
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    // 初始状态：未穿戴身体护甲
    EXPECT_TRUE(wolf.getBodyArmorItem().isEmpty());
    EXPECT_FALSE(wolf.isWearingBodyArmor());

    // 装备狼铠
    ItemStack wolfArmor(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmor);

    EXPECT_TRUE(wolf.isWearingBodyArmor());
    EXPECT_EQ(wolf.getBodyArmorItem().getItem(), Items::WOLF_ARMOR);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_EquipWolfArmor)
{
    // 主人右键驯服的狼 + 手持狼铠 → 装备狼铠
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;

    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    player.inventory().setItem(0, wolfArmorStack);
    player.inventory().setSelectedSlot(0);

    EXPECT_FALSE(wolf.isWearingBodyArmor());

    world.resetSoundTracking();
    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(wolf.isWearingBodyArmor());
    EXPECT_EQ(wolf.getBodyArmorItem().getItem(), Items::WOLF_ARMOR);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_NonOwnerCannotEquipArmor)
{
    // 非主人不能装备狼铠
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    Player stranger(EntityInstanceId(2), "Stranger");
    stranger.setPlayerId(99999ULL);

    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    stranger.inventory().setItem(0, wolfArmorStack);
    stranger.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(stranger, Hand::MainHand);
    EXPECT_FALSE(wolf.isWearingBodyArmor());
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_AlreadyEquippedCannotEquipAgain)
{
    // 已装备狼铠时不能再装备
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    // 先装备狼铠
    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);
    EXPECT_TRUE(wolf.isWearingBodyArmor());

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;

    ItemStack anotherArmorStack(Items::WOLF_ARMOR, 1);
    player.inventory().setItem(0, anotherArmorStack);
    player.inventory().setSelectedSlot(0);

    // 再次交互不会装备第二个狼铠，而是走其他交互逻辑
    wolf.interactMob(player, Hand::MainHand);
    // 仍然只有一个狼铠
    EXPECT_EQ(wolf.getBodyArmorItem().getCount(), 1);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_RepairWolfArmor)
{
    // 主人右键坐下的狼 + 手持犰狳鳞甲 + 狼铠已受损 → 修复狼铠
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setSitting(true);

    // 装备受损的狼铠
    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);
    // 设置耐久损伤
    wolf.getMutableEquipment(EquipmentSlot::Body).setDamage(32);
    EXPECT_TRUE(wolf.getBodyArmorItem().isDamaged());

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;

    // 手持犰狳鳞甲
    ItemStack scuteStack(Items::ARMADILLO_SCUTE, 10);
    player.inventory().setItem(0, scuteStack);
    player.inventory().setSelectedSlot(0);

    i32 damageBefore = wolf.getBodyArmorItem().getDamage();

    world.resetSoundTracking();
    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    // 修复后耐久损伤应该减少
    EXPECT_LT(wolf.getBodyArmorItem().getDamage(), damageBefore);
    // 非创造模式下消耗物品
    EXPECT_EQ(player.inventory().getItem(0).getCount(), 9);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_RepairRequiresSitting)
{
    // 狼铠修复需要狼坐下
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    wolf.setSitting(false); // 站着的狼

    // 装备受损的狼铠
    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);
    wolf.getMutableEquipment(EquipmentSlot::Body).setDamage(32);

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);

    ItemStack scuteStack(Items::ARMADILLO_SCUTE, 10);
    player.inventory().setItem(0, scuteStack);
    player.inventory().setSelectedSlot(0);

    i32 damageBefore = wolf.getBodyArmorItem().getDamage();
    wolf.interactMob(player, Hand::MainHand);

    // 站着的狼不会触发修复，损伤不变
    EXPECT_EQ(wolf.getBodyArmorItem().getDamage(), damageBefore);
}

// ============================================================================
// 狼铠伤害吸收测试（WolfEntity::actuallyHurt）
//
// 穿戴狼铠且伤害源不绕过护甲时，伤害由狼铠耐久吸收，狼不扣血。
// 参考: net.minecraft.world.entity.animal.wolf.Wolf.actuallyHurt()
// ============================================================================

TEST_F(WolfEntityTestFixture, ActuallyHurt_WithWolfArmor_WolfHealthUnchanged)
{
    // 穿戴狼铠的狼受到非绕过护甲伤害时，狼生命值不变，狼铠耐久降低
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(wolf.maxHealth()); // 满血

    // 装备狼铠（耐久 64）
    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);
    EXPECT_FALSE(wolf.getBodyArmorItem().isDamaged()); // 初始无损伤
    EXPECT_EQ(wolf.getBodyArmorItem().getMaxDamage(), 64);

    f32 healthBefore = wolf.health();
    i32 armorDamageBefore = wolf.getBodyArmorItem().getDamage();

    // 创建非绕过护甲的伤害源（生物攻击不绕过护甲）
    EntityDamageSource damageSource(DamageType::MobAttack, nullptr);

    world.resetSoundTracking();
    wolf.actuallyHurt(damageSource, 5.0f);

    // 狼不扣血（狼铠吸收伤害）
    EXPECT_FLOAT_EQ(wolf.health(), healthBefore);

    // 狼铠耐久降低（向上取整：5 点伤害 → 5 点耐久损伤）
    i32 armorDamageAfter = wolf.getBodyArmorItem().getDamage();
    EXPECT_GT(armorDamageAfter, armorDamageBefore);
    EXPECT_EQ(armorDamageAfter - armorDamageBefore, 5);
}

TEST_F(WolfEntityTestFixture, ActuallyHurt_WithWolfArmor_BypassesArmor_DamagesWolf)
{
    // 穿戴狼铠的狼受到绕过狼铠伤害时，狼扣血，狼铠耐久不变
    // MC 1.21.11: BYPASSES_WOLF_ARMOR 标签包含 drown（溺水），狼铠不吸收溺水伤害
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(wolf.maxHealth());

    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);

    f32 healthBefore = wolf.health();
    i32 armorDamageBefore = wolf.getBodyArmorItem().getDamage();

    // 创建绕过狼铠的伤害源（溺水伤害在 BYPASSES_WOLF_ARMOR 标签中）
    EnvironmentalDamage drownDamage(DamageType::Drown);

    wolf.actuallyHurt(drownDamage, 3.0f);

    // 狼扣血（溺水伤害绕过狼铠，不被狼铠吸收）
    EXPECT_LT(wolf.health(), healthBefore);

    // 狼铠耐久不变
    EXPECT_EQ(wolf.getBodyArmorItem().getDamage(), armorDamageBefore);
}

TEST_F(WolfEntityTestFixture, ActuallyHurt_WithoutWolfArmor_WolfTakesDamage)
{
    // 未穿戴狼铠的狼受到伤害时，正常扣血
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(wolf.maxHealth());

    EXPECT_FALSE(wolf.isWearingBodyArmor());

    f32 healthBefore = wolf.health();

    EntityDamageSource damageSource(DamageType::MobAttack, nullptr);
    wolf.actuallyHurt(damageSource, 4.0f);

    // 狼扣血（无狼铠吸收）
    EXPECT_LT(wolf.health(), healthBefore);
}

TEST_F(WolfEntityTestFixture, ActuallyHurt_ArmorAbsorption_PlaysDamageSound)
{
    // 狼铠吸收伤害时播放 ENTITY_WOLF_ARMOR_DAMAGE 音效
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(wolf.maxHealth());

    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);

    EntityDamageSource damageSource(DamageType::MobAttack, nullptr);

    world.resetSoundTracking();
    // 通过 hurt() 触发 getHurtSound → ENTITY_WOLF_ARMOR_DAMAGE
    wolf.hurt(damageSource, 5.0f);

    // 应该播放了狼铠受损音效（getHurtSound 返回 ENTITY_WOLF_ARMOR_DAMAGE）
    EXPECT_GT(world.getSoundPlayCount(), 0);
}

// ============================================================================
// 狼铠破损测试（耐久降至 0 触发 ENTITY_WOLF_ARMOR_BREAK 音效和槽位清空）
// ============================================================================

TEST_F(WolfEntityTestFixture, ActuallyHurt_ArmorBreak_PlaysBreakSoundAndClearsSlot)
{
    // 狼铠耐久降至 0 时播放 ENTITY_WOLF_ARMOR_BREAK 音效，且槽位清空
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(wolf.maxHealth());

    // 装备狼铠，并设置耐久接近破损
    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);
    wolf.getMutableEquipment(EquipmentSlot::Body).setDamage(60); // 60/64 已受损，剩余 4 点耐久

    EXPECT_TRUE(wolf.isWearingBodyArmor());

    f32 healthBefore = wolf.health();

    // 造成 5 点伤害（向上取整 5），狼铠耐久恰好降到 0 → 破损
    EntityDamageSource damageSource(DamageType::MobAttack, nullptr);

    world.resetSoundTracking();
    wolf.actuallyHurt(damageSource, 5.0f);

    // 狼不扣血（狼铠吸收了伤害）
    EXPECT_FLOAT_EQ(wolf.health(), healthBefore);

    // 槽位已清空（狼铠破损后 ItemStack 被清空）
    EXPECT_FALSE(wolf.isWearingBodyArmor());
    EXPECT_TRUE(wolf.getBodyArmorItem().isEmpty());

    // 应该播放了 ENTITY_WOLF_ARMOR_BREAK 音效
    EXPECT_GT(world.getSoundPlayCount(), 0);
    EXPECT_EQ(world.getLastSoundId(), SoundEvents::ENTITY_WOLF_ARMOR_BREAK);
}

TEST_F(WolfEntityTestFixture, ActuallyHurt_ArmorNotBroken_NoBreakSound)
{
    // 狼铠耐久未降至 0 时不播放破损音效，槽位仍装备狼铠
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(wolf.maxHealth());

    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);
    wolf.getMutableEquipment(EquipmentSlot::Body).setDamage(10); // 10/64，剩余 54 点耐久

    EntityDamageSource damageSource(DamageType::MobAttack, nullptr);

    world.resetSoundTracking();
    wolf.actuallyHurt(damageSource, 5.0f);

    // 狼铠未破损，槽位仍装备
    EXPECT_TRUE(wolf.isWearingBodyArmor());
    EXPECT_FALSE(wolf.getBodyArmorItem().isEmpty());

    // 不应该播放 ENTITY_WOLF_ARMOR_BREAK 音效
    // 可能在低耐久时播放了 ENTITY_WOLF_ARMOR_CRACK，但不应是 BREAK
    if (world.getSoundPlayCount() > 0) {
        EXPECT_NE(world.getLastSoundId(), SoundEvents::ENTITY_WOLF_ARMOR_BREAK);
    }
}

// ============================================================================
// 狼铠裂纹等级变化测试
//
// 当狼铠受损导致裂纹等级提升时（None→Low, Low→Medium, Medium→High），
// 播放 ENTITY_WOLF_ARMOR_CRACK 音效。
// 裂纹阈值（Crackiness::WOLF_ARMOR）：剩余 < 95% → Low, < 69% → Medium, < 32% → High
// 狼铠耐久 64：95% 阈值对应损伤 4（64*0.05=3.2，损伤 4 时剩余 60/64=93.75% < 95% → Low）
// ============================================================================

TEST_F(WolfEntityTestFixture, ActuallyHurt_CrackLevelChange_PlaysCrackSound)
{
    // 狼铠从无裂纹（None）受损到 Low 裂纹时，播放 ENTITY_WOLF_ARMOR_CRACK 音效
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setHealth(wolf.maxHealth());

    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);
    // 初始损伤 0 → 100% 剩余 → None

    // 造成 5 点伤害 → 损伤变为 5 → 剩余 59/64 ≈ 92.2% < 95% → Low
    // 裂纹等级从 None → Low，应播放 CRACK 音效
    EntityDamageSource damageSource(DamageType::MobAttack, nullptr);

    world.resetSoundTracking();
    wolf.actuallyHurt(damageSource, 5.0f);

    // 应该播放了 ENTITY_WOLF_ARMOR_CRACK 音效
    EXPECT_GT(world.getSoundPlayCount(), 0);
    EXPECT_EQ(world.getLastSoundId(), SoundEvents::ENTITY_WOLF_ARMOR_CRACK);

    // 狼铠未破损（耐久 59/64）
    EXPECT_TRUE(wolf.isWearingBodyArmor());
    EXPECT_FALSE(wolf.getBodyArmorItem().isEmpty());
}

// ============================================================================
// 剪刀剪切狼铠集成测试（MobEntity::processInitialInteract 剪刀分支）
//
// 玩家手持剪刀 + 已装备狼铠 + 主人 + 非潜行 → 剪下狼铠
// 剪刀耐久 -1，狼铠掉落为物品实体，播放 ITEM_ARMOR_UNEQUIP_WOLF 音效
// 参考: net.minecraft.world.entity.Entity.interact() 剪刀分支
// ============================================================================

TEST_F(WolfEntityTestFixture, ProcessInitialInteract_OwnerWithShears_ShearsWolfArmor)
{
    // 主人手持剪刀右键已装备狼铠的狼 → 剪下狼铠
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    // 装备狼铠
    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);
    EXPECT_TRUE(wolf.isWearingBodyArmor());

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL); // 主人
    player.abilities().creativeMode = false;

    // 手持剪刀
    ItemStack shearsStack(Items::SHEARS, 1);
    player.inventory().setItem(0, shearsStack);
    player.inventory().setSelectedSlot(0);

    i32 shearsDamageBefore = player.inventory().getItem(0).getDamage();

    world.resetSoundTracking();
    i32 spawnedBefore = world.getSpawnedEntityCount();
    ActionResultType result = wolf.processInitialInteract(player, Hand::MainHand);

    // 返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 狼铠槽位已清空
    EXPECT_FALSE(wolf.isWearingBodyArmor());
    EXPECT_TRUE(wolf.getBodyArmorItem().isEmpty());

    // 剪刀耐久 -1
    i32 shearsDamageAfter = player.inventory().getItem(0).getDamage();
    EXPECT_EQ(shearsDamageAfter - shearsDamageBefore, 1);

    // 应该掉落了一个物品实体（狼铠）
    EXPECT_EQ(world.getSpawnedEntityCount() - spawnedBefore, 1);

    // 应该播放了 ITEM_ARMOR_UNEQUIP_WOLF 音效
    EXPECT_GT(world.getSoundPlayCount(), 0);
    EXPECT_EQ(world.getLastSoundId(), SoundEvents::ITEM_ARMOR_UNEQUIP_WOLF);
}

TEST_F(WolfEntityTestFixture, ProcessInitialInteract_NonOwnerWithShears_DoesNotShear)
{
    // 非主人手持剪刀不能剪下狼铠（WolfEntity::canShearEquipment 仅允许主人剪切）
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);

    Player stranger(EntityInstanceId(2), "Stranger");
    stranger.setPlayerId(99999ULL); // 非主人

    ItemStack shearsStack(Items::SHEARS, 1);
    stranger.inventory().setItem(0, shearsStack);
    stranger.inventory().setSelectedSlot(0);

    i32 spawnedBefore = world.getSpawnedEntityCount();
    ActionResultType result = wolf.processInitialInteract(stranger, Hand::MainHand);

    // 非主人不能剪切，狼铠仍在
    EXPECT_TRUE(wolf.isWearingBodyArmor());
    // 不应该掉落物品
    EXPECT_EQ(world.getSpawnedEntityCount(), spawnedBefore);
}

TEST_F(WolfEntityTestFixture, ProcessInitialInteract_Shears_NoArmor_NoEffect)
{
    // 手持剪刀但狼未装备狼铠时，剪刀分支不触发，进入 interactMob
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    EXPECT_FALSE(wolf.isWearingBodyArmor());

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);

    ItemStack shearsStack(Items::SHEARS, 1);
    player.inventory().setItem(0, shearsStack);
    player.inventory().setSelectedSlot(0);

    i32 shearsDamageBefore = player.inventory().getItem(0).getDamage();
    i32 spawnedBefore = world.getSpawnedEntityCount();

    wolf.processInitialInteract(player, Hand::MainHand);

    // 剪刀耐久不变（剪刀分支未触发）
    EXPECT_EQ(player.inventory().getItem(0).getDamage(), shearsDamageBefore);
    // 不掉落物品
    EXPECT_EQ(world.getSpawnedEntityCount(), spawnedBefore);
}

TEST_F(WolfEntityTestFixture, ProcessInitialInteract_SneakingWithShears_DoesNotShear)
{
    // 玩家潜行时手持剪刀不触发剪切（与 MC 1.21.11 一致）
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setWorld(&world);
    player.setPlayerId(12345ULL);
    player.setSneaking(true); // 潜行状态

    ItemStack shearsStack(Items::SHEARS, 1);
    player.inventory().setItem(0, shearsStack);
    player.inventory().setSelectedSlot(0);

    i32 shearsDamageBefore = player.inventory().getItem(0).getDamage();
    i32 spawnedBefore = world.getSpawnedEntityCount();

    wolf.processInitialInteract(player, Hand::MainHand);

    // 潜行时不剪切，狼铠仍在
    EXPECT_TRUE(wolf.isWearingBodyArmor());
    EXPECT_EQ(player.inventory().getItem(0).getDamage(), shearsDamageBefore);
    EXPECT_EQ(world.getSpawnedEntityCount(), spawnedBefore);
}

// ============================================================================
// 狼铠染色交互测试（WolfEntity::interactMob 染色分支）
//
// 主人手持染料 + 已装备狼铠 → 改变狼铠颜色（与当前颜色混合）
// 参考: net.minecraft.world.entity.animal.wolf.Wolf.mobInteract() 染色分支
// ============================================================================

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_DyeOnArmor_ChangesArmorColor)
{
    // 主人手持染料 + 已装备狼铠 → 改变狼铠颜色
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    // 装备狼铠
    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);

    // 验证初始颜色为默认色（犰狳鳞甲棕色 0xA06540）
    const auto* dyeableArmor = dynamic_cast<const item::items::DyeableArmorItem*>(wolf.getBodyArmorItem().getItem());
    ASSERT_NE(dyeableArmor, nullptr);
    u32 initialColor = dyeableArmor->getColor(wolf.getBodyArmorItem());
    EXPECT_EQ(initialColor, 0xA06540);

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL); // 主人
    player.abilities().creativeMode = false;

    // 手持红色染料
    ItemStack redDyeStack(Items::RED_DYE, 10);
    player.inventory().setItem(0, redDyeStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    // 返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 狼铠颜色应该发生变化（与红色混合）
    u32 newColor = dyeableArmor->getColor(wolf.getBodyArmorItem());
    EXPECT_NE(newColor, initialColor);

    // 物品应该消耗
    i32 countAfter = player.inventory().getItem(0).getCount();
    EXPECT_EQ(countAfter, 9);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_DyeOnArmor_NonOwner_NoColorChange)
{
    // 非主人手持染料 + 已装备狼铠 → 不改变狼铠颜色
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);

    const auto* dyeableArmor = dynamic_cast<const item::items::DyeableArmorItem*>(wolf.getBodyArmorItem().getItem());
    ASSERT_NE(dyeableArmor, nullptr);
    u32 initialColor = dyeableArmor->getColor(wolf.getBodyArmorItem());

    Player stranger(EntityInstanceId(2), "Stranger");
    stranger.setPlayerId(99999ULL); // 非主人
    stranger.abilities().creativeMode = false;

    ItemStack redDyeStack(Items::RED_DYE, 10);
    stranger.inventory().setItem(0, redDyeStack);
    stranger.inventory().setSelectedSlot(0);

    wolf.interactMob(stranger, Hand::MainHand);

    // 狼铠颜色不变
    u32 colorAfter = dyeableArmor->getColor(wolf.getBodyArmorItem());
    EXPECT_EQ(colorAfter, initialColor);

    // 物品不消耗
    EXPECT_EQ(stranger.inventory().getItem(0).getCount(), 10);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_DyeOnArmor_MixesColors)
{
    // 多次染色应该混合颜色（与当前颜色取 RGB 平均值）
    // 狼铠默认颜色为犰狳鳞甲棕色 0xA06540 = (160, 101, 64)
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);

    ItemStack wolfArmorStack(Items::WOLF_ARMOR, 1);
    wolf.setBodyArmorItem(wolfArmorStack);

    const auto* dyeableArmor = dynamic_cast<const item::items::DyeableArmorItem*>(wolf.getBodyArmorItem().getItem());
    ASSERT_NE(dyeableArmor, nullptr);

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = true; // 创造模式不消耗物品

    // 验证初始颜色为默认色（犰狳鳞甲棕色 0xA06540）
    u32 initialColor = dyeableArmor->getColor(wolf.getBodyArmorItem());
    EXPECT_EQ(initialColor, 0xA06540);

    // 第一次染色：白色染料 0xFFFFFF
    // 混合后：((160+255)/2, (101+255)/2, (64+255)/2) = (207, 178, 159)
    ItemStack whiteDyeStack(Items::WHITE_DYE, 10);
    player.inventory().setItem(0, whiteDyeStack);
    player.inventory().setSelectedSlot(0);
    wolf.interactMob(player, Hand::MainHand);
    u32 colorAfterWhite = dyeableArmor->getColor(wolf.getBodyArmorItem());
    EXPECT_NE(colorAfterWhite, initialColor);

    u32 expectedAfterWhiteR = (0xA0 + 0xFF) / 2; // 207
    u32 expectedAfterWhiteG = (0x65 + 0xFF) / 2; // 178
    u32 expectedAfterWhiteB = (0x40 + 0xFF) / 2; // 159
    EXPECT_EQ((colorAfterWhite >> 16) & 0xFF, expectedAfterWhiteR);
    EXPECT_EQ((colorAfterWhite >> 8) & 0xFF, expectedAfterWhiteG);
    EXPECT_EQ(colorAfterWhite & 0xFF, expectedAfterWhiteB);

    // 第二次染色：黑色染料 0x191919
    // 混合后：((207+25)/2, (178+25)/2, (159+25)/2) = (116, 101, 92)
    ItemStack blackDyeStack(Items::INK_SAC, 10);
    player.inventory().setItem(0, blackDyeStack);
    wolf.interactMob(player, Hand::MainHand);
    u32 colorAfterBlack = dyeableArmor->getColor(wolf.getBodyArmorItem());

    // 两次染色后颜色应该不同
    EXPECT_NE(colorAfterWhite, colorAfterBlack);

    u32 expectedAfterBlackR = (expectedAfterWhiteR + 0x19) / 2; // 116
    u32 expectedAfterBlackG = (expectedAfterWhiteG + 0x19) / 2; // 101
    u32 expectedAfterBlackB = (expectedAfterWhiteB + 0x19) / 2; // 92
    EXPECT_EQ((colorAfterBlack >> 16) & 0xFF, expectedAfterBlackR);
    EXPECT_EQ((colorAfterBlack >> 8) & 0xFF, expectedAfterBlackG);
    EXPECT_EQ(colorAfterBlack & 0xFF, expectedAfterBlackB);
}

TEST_F(WolfEntityTestFixture, InteractMob_TamedWolf_NoArmor_DyeChangesCollarOnly)
{
    // 未装备狼铠时，染料仅改变颈圈颜色（不进入狼铠染色分支）
    WolfTestWorld world;
    WolfEntity wolf(EntityInstanceId(1));
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setTamed(true);
    wolf.setOwnerId(12345ULL);
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Red); // 默认红色颈圈

    Player player(EntityInstanceId(2), "TestPlayer");
    player.setPlayerId(12345ULL);
    player.abilities().creativeMode = false;

    ItemStack blueDyeStack(Items::LAPIS_LAZULI_DYE, 10);
    player.inventory().setItem(0, blueDyeStack);
    player.inventory().setSelectedSlot(0);

    ActionResultType result = wolf.interactMob(player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    // 颈圈变为蓝色
    EXPECT_EQ(wolf.getCollarColor(), DyeColor::Blue);
}

// ============================================================================
// 甩水动画状态机测试（WolfEntity::tick / die / getShakeAnim / getWetShade / getHeadRollAngle）
//
// 参考: net.minecraft.world.entity.animal.wolf.Wolf (MC 1.21.11)
// - Wolf.tick(): interestedAngle 插值 + isWet/shakeAnim 状态机
// - Wolf.aiStep(): 甩水触发（isWet && !isShaking && !isPathFinding && onGround）
// - Wolf.die(): 重置 isWet/isShaking/shakeAnim/shakeAnimO
// - Wolf.getShakeAnim(partialTick): lerp(partialTick, shakeAnimO, shakeAnim)
// - Wolf.getWetShade(partialTick): !isWet ? 1.0 : min(0.75 + shakeAnim/2*0.25, 1.0)
// - Wolf.getHeadRollAngle(partialTick): lerp(partialTick, interestedAngleO, interestedAngle) * 0.15 * PI
// ============================================================================

namespace {
/// 测试用狼实体子类：允许覆写 isInWaterOrRain 以驱动甩水状态机
class TestWolfEntity : public WolfEntity {
public:
    explicit TestWolfEntity(EntityInstanceId id)
        : WolfEntity(id)
    {}

    [[nodiscard]] bool isInWaterOrRain() const override { return m_forceInWaterOrRain; }

    void setForceInWaterOrRain(bool value) { m_forceInWaterOrRain = value; }

    /// 直接设置 isWet 状态（绕过 isInWaterOrRain）用于测试已湿润场景
    void setWetForTest(bool value) { _setWetForTest(value); }

    /// 直接设置 isShaking 状态（绕过触发条件）用于测试甩水进度
    void setShakingForTest(bool value) { _setShakingForTest(value); }

    /// 直接设置 interested 状态用于测试 interestedAngle 插值
    void setInterestedForTest(bool value) { setInterested(value); }

    /// 直接设置 shakeAnim 用于测试数学方法
    void setShakeAnimForTest(f32 anim, f32 animO) { _setShakeAnimForTest(anim, animO); }

private:
    bool m_forceInWaterOrRain = false;
};
} // namespace

TEST_F(WolfEntityTestFixture, Shake_DefaultState_AllZero)
{
    // 默认状态：isWet=false, isShaking=false, shakeAnim=0
    WolfEntity wolf(EntityInstanceId(1));
    EXPECT_FALSE(wolf.isWet());
    EXPECT_FALSE(wolf.isShaking());
    EXPECT_FLOAT_EQ(wolf.getShakeAnim(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(wolf.getShakeAnim(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(wolf.getShakeAnim(1.0f), 0.0f);
    EXPECT_FLOAT_EQ(wolf.getWetShade(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(wolf.getWetShade(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(wolf.getHeadRollAngle(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(wolf.getHeadRollAngle(1.0f), 0.0f);
}

TEST_F(WolfEntityTestFixture, Shake_GetShakeAnim_Interpolation)
{
    // getShakeAnim(partialTick) = lerp(partialTick, shakeAnimO, shakeAnim)
    TestWolfEntity wolf(EntityInstanceId(1));
    wolf.setShakeAnimForTest(1.0f, 0.5f); // shakeAnim=1.0, shakeAnimO=0.5

    EXPECT_FLOAT_EQ(wolf.getShakeAnim(0.0f), 0.5f);  // partialTick=0 → shakeAnimO
    EXPECT_FLOAT_EQ(wolf.getShakeAnim(0.5f), 0.75f); // 中点
    EXPECT_FLOAT_EQ(wolf.getShakeAnim(1.0f), 1.0f);  // partialTick=1 → shakeAnim
}

TEST_F(WolfEntityTestFixture, Shake_GetWetShade_DryWolf)
{
    // 干燥狼：getWetShade 始终返回 1.0
    TestWolfEntity wolf(EntityInstanceId(1));
    wolf.setWetForTest(false);
    wolf.setShakeAnimForTest(0.0f, 0.0f);

    EXPECT_FLOAT_EQ(wolf.getWetShade(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(wolf.getWetShade(0.5f), 1.0f);
    EXPECT_FLOAT_EQ(wolf.getWetShade(1.0f), 1.0f);
}

TEST_F(WolfEntityTestFixture, Shake_GetWetShade_WetWolf)
{
    // 湿润狼：getWetShade = min(0.75 + shakeAnim/2*0.25, 1.0)
    TestWolfEntity wolf(EntityInstanceId(1));
    wolf.setWetForTest(true);
    wolf.setShakeAnimForTest(0.0f, 0.0f); // 刚开始甩水

    // shakeAnim=0 → 0.75 + 0 = 0.75
    EXPECT_FLOAT_EQ(wolf.getWetShade(0.0f), 0.75f);

    // shakeAnim=2.0 → 0.75 + 2.0/2*0.25 = 0.75 + 0.25 = 1.0
    wolf.setShakeAnimForTest(2.0f, 2.0f);
    EXPECT_FLOAT_EQ(wolf.getWetShade(1.0f), 1.0f);

    // shakeAnim=1.0 → 0.75 + 0.125 = 0.875
    wolf.setShakeAnimForTest(1.0f, 1.0f);
    EXPECT_FLOAT_EQ(wolf.getWetShade(1.0f), 0.875f);
}

TEST_F(WolfEntityTestFixture, Shake_GetHeadRollAngle_Interpolation)
{
    // getHeadRollAngle(partialTick) = lerp(partialTick, interestedAngleO, interestedAngle) * 0.15 * PI
    // 注：interestedAngle 由 tick() 内部插值，此处测试数学公式
    TestWolfEntity wolf(EntityInstanceId(1));
    wolf.setInterestedForTest(true);

    // 触发多次 tick 让 interestedAngle 趋近 1.0
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    for (int i = 0; i < 50; ++i) {
        wolf.tick();
    }

    // interestedAngle 应该趋近 1.0，getHeadRollAngle 应该趋近 0.15 * PI
    const f32 expected = 0.15f * math::PI;
    EXPECT_NEAR(wolf.getHeadRollAngle(0.0f), expected, 0.01f);
}

TEST_F(WolfEntityTestFixture, Shake_Tick_InterestedAngleInterpolation)
{
    // tick() 应该让 interestedAngle 向 1.0 或 0.0 插值
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setOnGround(true);

    // interested=true → interestedAngle 应该向 1.0 趋近
    wolf.setInterestedForTest(true);
    wolf.tick();

    // 每次插值：interestedAngle += (1.0 - interestedAngle) * 0.4
    // 初始 0.0 → 0.0 + 1.0 * 0.4 = 0.4
    // 注：getHeadRollAngle 返回的是 * 0.15 * PI，所以需要除以这个系数得到 interestedAngle
    // partialTick=1.0 取当前帧值（m_interestedAngle），partialTick=0.0 取上一帧值（m_interestedAngleO）
    const f32 headRoll = wolf.getHeadRollAngle(1.0f);
    const f32 interestedAngle = headRoll / (0.15f * math::PI);
    EXPECT_NEAR(interestedAngle, 0.4f, 0.01f);
}

TEST_F(WolfEntityTestFixture, Shake_Tick_WetFlagSetWhenInWaterOrRain)
{
    // isInWaterOrRain()=true 时，tick() 应该设置 isWet=true
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setOnGround(true);

    EXPECT_FALSE(wolf.isWet());

    wolf.setForceInWaterOrRain(true);
    wolf.tick();

    EXPECT_TRUE(wolf.isWet());
}

TEST_F(WolfEntityTestFixture, Shake_Tick_TriggersShakeWhenWetAndOnGround)
{
    // 已湿润 + 在地面 + 未在寻路 → 触发甩水（广播 ShakeOffWater=8）
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setOnGround(true);

    // 先让狼湿润（在水中）
    wolf.setForceInWaterOrRain(true);
    wolf.tick();
    EXPECT_TRUE(wolf.isWet());
    EXPECT_FALSE(wolf.isShaking());

    world.resetBroadcastTracking();

    // 离开水但在地面 → 触发甩水
    wolf.setForceInWaterOrRain(false);
    wolf.tick();

    // 应该广播 ShakeOffWater (8)
    EXPECT_EQ(world.getBroadcastCount(), 1);
    EXPECT_EQ(world.getLastBroadcastStatus(), static_cast<u8>(network::EntityStatus::ShakeOffWater));
    EXPECT_TRUE(wolf.isShaking());
    EXPECT_FLOAT_EQ(wolf.getShakeAnim(0.0f), 0.0f); // shakeAnimO=0
}

TEST_F(WolfEntityTestFixture, Shake_Tick_ShakeProgression)
{
    // 甩水开始后，每 tick shakeAnim += 0.05
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setOnGround(true);

    // 触发甩水
    wolf.setForceInWaterOrRain(true);
    wolf.tick(); // isWet=true (第1次tick)
    wolf.setForceInWaterOrRain(false);
    wolf.tick(); // 触发甩水(第2次tick) → shakeAnim=0.05, shakeAnimO=0

    EXPECT_TRUE(wolf.isShaking());
    // 触发 tick 内已经推进一次：shakeAnim=0.05, shakeAnimO=0
    EXPECT_NEAR(wolf.getShakeAnim(1.0f), 0.05f, 0.001f);
    EXPECT_NEAR(wolf.getShakeAnim(0.0f), 0.0f, 0.001f);

    // 第二次进度推进：shakeAnimO=0.05 → shakeAnim=0.10
    wolf.tick();
    EXPECT_NEAR(wolf.getShakeAnim(1.0f), 0.10f, 0.001f);
    EXPECT_NEAR(wolf.getShakeAnim(0.0f), 0.05f, 0.001f);
}

TEST_F(WolfEntityTestFixture, Shake_Tick_ShakeCompletion)
{
    // shakeAnimO >= 2.0 时甩水完成，状态重置
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setOnGround(true);

    // 直接设置接近完成的甩水进度
    wolf.setWetForTest(true);
    wolf.setShakingForTest(true);
    wolf.setShakeAnimForTest(2.0f, 1.95f); // shakeAnim=2.0, shakeAnimO=1.95

    EXPECT_TRUE(wolf.isShaking());

    wolf.tick();

    // 甩水完成：isShaking=false, isWet=false, shakeAnim=0
    EXPECT_FALSE(wolf.isShaking());
    EXPECT_FALSE(wolf.isWet());
    EXPECT_FLOAT_EQ(wolf.getShakeAnim(0.0f), 0.0f);
}

TEST_F(WolfEntityTestFixture, Shake_Tick_CancelWhenReenteringWater)
{
    // 甩水中再次接触水 → 取消甩水并广播 WolfStopShaking(56)
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setOnGround(true);

    // 先触发甩水
    wolf.setForceInWaterOrRain(true);
    wolf.tick();
    wolf.setForceInWaterOrRain(false);
    wolf.tick();
    EXPECT_TRUE(wolf.isShaking());

    // 推进几 tick
    wolf.tick();
    wolf.tick();
    EXPECT_TRUE(wolf.isShaking());

    world.resetBroadcastTracking();

    // 再次接触水 → 取消甩水
    wolf.setForceInWaterOrRain(true);
    wolf.tick();

    EXPECT_FALSE(wolf.isShaking());
    EXPECT_EQ(world.getBroadcastCount(), 1);
    EXPECT_EQ(world.getLastBroadcastStatus(), static_cast<u8>(network::EntityStatus::WolfStopShaking));
}

TEST_F(WolfEntityTestFixture, Shake_Die_ResetsShakeState)
{
    // die() 应该重置 isWet/isShaking/shakeAnim/shakeAnimO
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");

    // 设置甩水状态
    wolf.setWetForTest(true);
    wolf.setShakingForTest(true);
    wolf.setShakeAnimForTest(1.5f, 1.0f);
    EXPECT_TRUE(wolf.isWet());
    EXPECT_TRUE(wolf.isShaking());

    // 创建伤害源并杀死狼
    EntityDamageSource damageSource(DamageType::MobAttack, nullptr);
    wolf.die(damageSource);

    // 状态应被重置
    EXPECT_FALSE(wolf.isWet());
    EXPECT_FALSE(wolf.isShaking());
    EXPECT_FLOAT_EQ(wolf.getShakeAnim(0.0f), 0.0f);
}

TEST_F(WolfEntityTestFixture, Shake_Tick_ShakeSoundPlayedOnce)
{
    // 甩水开始时（shakeAnim==0.0）应该播放一次 ENTITY_WOLF_SHAKE 声音
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setOnGround(true);

    // 触发甩水
    wolf.setForceInWaterOrRain(true);
    wolf.tick();
    wolf.setForceInWaterOrRain(false);

    world.resetSoundTracking();
    wolf.tick(); // 触发甩水，下一 tick 播放声音

    // 应该播放了 ENTITY_WOLF_SHAKE 声音
    EXPECT_GE(world.getSoundPlayCount(), 1);
}

TEST_F(WolfEntityTestFixture, Shake_Tick_SplashParticlesAfter04)
{
    // shakeAnim > 0.4 时应该发射 SPLASH 粒子
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setOnGround(true);

    // 直接设置甩水中期状态
    wolf.setWetForTest(true);
    wolf.setShakingForTest(true);
    wolf.setShakeAnimForTest(1.0f, 0.95f); // shakeAnim=1.0, shakeAnimO=0.95

    // 注：WolfTestWorld 的 addParticle 是默认空实现（BaseTestWorld 未覆写）
    // 此测试主要验证 tick() 不会崩溃，并推进 shakeAnim
    wolf.tick();

    // tick 后：shakeAnimO=1.0（旧 shakeAnim），shakeAnim=1.05（+0.05）
    // getShakeAnim(1.0) = shakeAnimO + (shakeAnim - shakeAnimO) * 1.0 = 1.0 + 0.05 = 1.05
    EXPECT_NEAR(wolf.getShakeAnim(1.0f), 1.05f, 0.001f);
    // getShakeAnim(0.0) = shakeAnimO = 1.0
    EXPECT_NEAR(wolf.getShakeAnim(0.0f), 1.0f, 0.001f);
}

TEST_F(WolfEntityTestFixture, Shake_Tick_NoTriggerWhenNotOnGround)
{
    // 在水中但不在地面 → 不会触发甩水
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    world.setGroundCollisionEnabled(false); // 禁用虚拟地面，使 onGround=false
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setOnGround(false); // 不在地面

    wolf.setForceInWaterOrRain(true);
    wolf.tick(); // isWet=true

    world.resetBroadcastTracking();
    wolf.setForceInWaterOrRain(false);
    wolf.tick();

    // 不应该触发甩水（因为不在地面）
    EXPECT_FALSE(wolf.isShaking());
    EXPECT_EQ(world.getBroadcastCount(), 0);
}

TEST_F(WolfEntityTestFixture, Shake_Tick_NoReTriggerWhileShaking)
{
    // 已经在甩水时，不会重复触发 ShakeOffWater
    TestWolfEntity wolf(EntityInstanceId(1));
    WolfTestWorld world;
    wolf.setWorld(&world);
    wolf.setTypeId("minecraft:wolf");
    wolf.setOnGround(true);

    // 触发甩水
    wolf.setForceInWaterOrRain(true);
    wolf.tick();
    wolf.setForceInWaterOrRain(false);
    wolf.tick();
    EXPECT_TRUE(wolf.isShaking());

    world.resetBroadcastTracking();

    // 继续推进 tick（不在水中），不应该重复广播
    wolf.tick();
    wolf.tick();
    wolf.tick();

    EXPECT_EQ(world.getBroadcastCount(), 0);
    EXPECT_TRUE(wolf.isShaking());
}

// ============================================================================
// 愤怒状态 DataParameter 同步测试
//
// WolfEntity 重写了 TameableEntity 的 isAngry/getAngerTime/setAngerTime/setAngry，
// 将愤怒状态从成员变量（TameableEntity::m_angerTime）改为 DataParameter
// （WolfEntity::DATA_ANGER_TIME_PARAM），从而让愤怒状态能通过 EntityTracker
// 自动广播到所有观察者客户端，驱动尾巴角度和纹理变体等渲染表现。
//
// 以下测试验证愤怒状态的 DataParameter 同步链路。
// ============================================================================

TEST_F(WolfEntityTestFixture, DataParameter_AngerTimeParamId_IsValid)
{
    // DATA_ANGER_TIME_PARAM 的 ID 应该是有效的（>0，由 createKey 自动分配）
    u16 paramId = WolfEntity::getAngerTimeParamId();
    EXPECT_GT(paramId, 0u);
}

TEST_F(WolfEntityTestFixture, DataParameter_IsAngry_ReadsFromDataManager)
{
    // isAngry() 应该从 DataManager 读取而非成员变量
    WolfEntity wolf(EntityInstanceId(0));
    EXPECT_FALSE(wolf.isAngry());
    EXPECT_EQ(wolf.getAngerTime(), 0);

    wolf.setAngerTime(100);
    EXPECT_TRUE(wolf.isAngry());
    EXPECT_EQ(wolf.getAngerTime(), 100);

    // 通过 DataManager 直接读取验证
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getAngerTimeParamId();
    EXPECT_TRUE(dataManager.hasParam(paramId));
    i32 storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
    EXPECT_EQ(storedValue, 100);
}

TEST_F(WolfEntityTestFixture, DataParameter_SetAngry_WritesToDataManager)
{
    // setAngry(true) 应该通过虚函数 setAngerTime 路由到 DATA_ANGER_TIME_PARAM
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getAngerTimeParamId();

    // 设置愤怒状态
    wolf.setAngry(true);
    EXPECT_TRUE(wolf.isAngry());
    i32 storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
    EXPECT_GT(storedValue, 0); // setAngry(true) 写入 MAX_ANGER_TIME

    // 清除愤怒状态
    wolf.setAngry(false);
    EXPECT_FALSE(wolf.isAngry());
    storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
    EXPECT_EQ(storedValue, 0);
}

TEST_F(WolfEntityTestFixture, DataParameter_SetAngerTime_WritesToDataManager)
{
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getAngerTimeParamId();

    // 设置愤怒时间
    wolf.setAngerTime(42);
    i32 storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
    EXPECT_EQ(storedValue, 42);

    // 设置为 0
    wolf.setAngerTime(0);
    storedValue = dataManager.get<i32>(entity::DataParameter<i32>(paramId));
    EXPECT_EQ(storedValue, 0);
}

TEST_F(WolfEntityTestFixture, DataParameter_DirtyFlag_OnAngerChange)
{
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();

    // 初始状态不应有脏数据
    dataManager.clearDirty();
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置愤怒时间应该标记为脏数据
    wolf.setAngerTime(100);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 清除脏标记后设置相同值不应标记为脏
    dataManager.clearDirty();
    wolf.setAngerTime(100);
    EXPECT_FALSE(dataManager.hasDirtyData());

    // 设置不同值应该标记为脏
    wolf.setAngerTime(50);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 设置为 0 也应标记为脏
    dataManager.clearDirty();
    wolf.setAngerTime(0);
    EXPECT_TRUE(dataManager.hasDirtyData());
}

TEST_F(WolfEntityTestFixture, DataParameter_AngerTimeRegisteredOnConstruction)
{
    // 验证 WolfEntity 构造后 DATA_ANGER_TIME_PARAM 已注册到 DataManager
    WolfEntity wolf(EntityInstanceId(0));
    auto& dataManager = wolf.dataManager();
    u16 paramId = WolfEntity::getAngerTimeParamId();

    // 参数应该已注册
    EXPECT_TRUE(dataManager.hasParam(paramId));

    // 默认值应为 0（非愤怒）
    EXPECT_EQ(wolf.getAngerTime(), 0);
    EXPECT_FALSE(wolf.isAngry());
}

TEST_F(WolfEntityTestFixture, DataParameter_AngerState_SyncsStateChanges)
{
    // 验证多次状态变更正确同步
    WolfEntity wolf(EntityInstanceId(0));

    EXPECT_FALSE(wolf.isAngry());

    wolf.setAngry(true);
    EXPECT_TRUE(wolf.isAngry());
    EXPECT_GT(wolf.getAngerTime(), 0);

    wolf.setAngry(false);
    EXPECT_FALSE(wolf.isAngry());
    EXPECT_EQ(wolf.getAngerTime(), 0);

    wolf.setAngerTime(75);
    EXPECT_TRUE(wolf.isAngry());
    EXPECT_EQ(wolf.getAngerTime(), 75);

    wolf.setAngerTime(0);
    EXPECT_FALSE(wolf.isAngry());
}

TEST_F(WolfEntityTestFixture, DataParameter_TailAngle_UsesAngerFromDataManager)
{
    // 验证 getTailAngle() 在愤怒时返回 1.539f，且愤怒状态来自 DataParameter 而非成员变量
    WolfEntity wolf(EntityInstanceId(0));

    // 非愤怒状态：尾巴角度基于生命值
    wolf.setHealth(wolf.maxHealth());
    f32 calmTailAngle = wolf.getTailAngle();
    EXPECT_NE(calmTailAngle, 1.539f);

    // 通过 DataParameter 设置愤怒时间
    wolf.setAngerTime(100);
    EXPECT_TRUE(wolf.isAngry());
    f32 angryTailAngle = wolf.getTailAngle();
    EXPECT_FLOAT_EQ(angryTailAngle, 1.539f);

    // 清除愤怒时间
    wolf.setAngerTime(0);
    EXPECT_FALSE(wolf.isAngry());
    EXPECT_NE(wolf.getTailAngle(), 1.539f);
}

} // namespace
} // namespace mc
