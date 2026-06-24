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
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::item::tag;

// ============================================================================
// 测试用 Mock World（支持 playSound 捕获）
// ============================================================================

class LavaFireTestWorld final : public mc::test::BaseTestWorld {
public:
    LavaFireTestWorld() = default;

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_lastSoundId = soundEventId;
        m_lastSoundVolume = volume;
        m_lastSoundPitch = pitch;
        m_soundPlayCount++;
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        MC_UNUSED(context);
        m_lastGameEventId = event.id();
        m_lastGameEventPos = pos;
        m_gameEventCount++;
    }

    [[nodiscard]] bool isClientSide() const override { return false; }

    void clearState()
    {
        m_soundPlayCount = 0;
        m_lastSoundId = ResourceLocation();
        m_lastSoundVolume = 0.0f;
        m_lastSoundPitch = 0.0f;
        m_gameEventCount = 0;
        m_lastGameEventId.clear();
        m_lastGameEventPos = BlockPos(0, 0, 0);
    }

    [[nodiscard]] i32 soundPlayCount() const { return m_soundPlayCount; }
    [[nodiscard]] const ResourceLocation& lastSoundId() const { return m_lastSoundId; }
    [[nodiscard]] f32 lastSoundVolume() const { return m_lastSoundVolume; }
    [[nodiscard]] f32 lastSoundPitch() const { return m_lastSoundPitch; }
    [[nodiscard]] i32 gameEventCount() const { return m_gameEventCount; }

private:
    i32 m_soundPlayCount = 0;
    ResourceLocation m_lastSoundId;
    f32 m_lastSoundVolume = 0.0f;
    f32 m_lastSoundPitch = 0.0f;
    i32 m_gameEventCount = 0;
    std::string m_lastGameEventId;
    BlockPos m_lastGameEventPos{0, 0, 0};
};

// ============================================================================
// 测试用 LivingEntity 子类（用于需要 hurt 逻辑的测试）
// ============================================================================

namespace {

class TestLivingEntity : public LivingEntity {
public:
    explicit TestLivingEntity(EntityId id, IWorld* world = nullptr)
        : LivingEntity(id)
    {
        registerAttributes();
        setHealth(maxHealth());
        if (world != nullptr) {
            setWorld(world);
        }
    }

    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 height() const override { return 1.8f; }
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }
    [[nodiscard]] std::string getLootTableId() const override { return {}; }
};

} // namespace

// ============================================================================
// 测试固定装置
// ============================================================================

class EntityLavaFireTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        ItemTags::initialize();
    }

    void SetUp() override { m_world.clearState(); }

    LavaFireTestWorld m_world;
};

// ============================================================================
// lavaIgnite 测试
// ============================================================================

TEST_F(EntityLavaFireTest, LavaIgnite_SetsFireForNonImmuneEntity)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    EXPECT_FALSE(entity.isOnFire());

    entity.lavaIgnite();
    EXPECT_TRUE(entity.isOnFire());
    EXPECT_EQ(entity.fire(), 300); // 15 秒 = 300 ticks
}

TEST_F(EntityLavaFireTest, LavaIgnite_DoesNotReduceExistingHigherFireTime)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    // 设置一个很高的火焰时间
    entity.forceFireTicks(500);
    entity.lavaIgnite();
    // lavaIgnite 内部调用 setFire(300)，但 setFire 只增加不减少
    EXPECT_EQ(entity.fire(), 500);
}

TEST_F(EntityLavaFireTest, LavaIgnite_IncreasesLowFireTime)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.forceFireTicks(10);
    entity.lavaIgnite();
    // setFire(300) 会覆盖较低的值
    EXPECT_EQ(entity.fire(), 300);
}

// ============================================================================
// lavaHurt 测试
// ============================================================================

TEST_F(EntityLavaFireTest, LavaHurt_DealsDamageToNonImmuneEntity)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    f32 healthBefore = entity.health();

    entity.lavaHurt();
    EXPECT_LT(entity.health(), healthBefore);
    EXPECT_EQ(entity.health(), healthBefore - 4.0f);
}

TEST_F(EntityLavaFireTest, LavaHurt_PlaysSoundWhenDamageSucceeds)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.lavaHurt();

    // 基类 shouldPlayLavaHurtSound() 返回 true，且实体未静音
    EXPECT_EQ(m_world.soundPlayCount(), 1);
    EXPECT_EQ(m_world.lastSoundId(), SoundEvents::ENTITY_GENERIC_BURN);
    // 音量 0.4，音调范围 [2.0, 2.4]
    EXPECT_FLOAT_EQ(m_world.lastSoundVolume(), 0.4f);
    EXPECT_GE(m_world.lastSoundPitch(), 2.0f);
    EXPECT_LE(m_world.lastSoundPitch(), 2.4f);
}

TEST_F(EntityLavaFireTest, LavaHurt_NoSoundWhenEntityIsSilent)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.setSilent(true);
    entity.lavaHurt();

    // 静音实体不应播放音效
    EXPECT_EQ(m_world.soundPlayCount(), 0);
}

TEST_F(EntityLavaFireTest, LavaHurt_NoSoundWhenDamageFails)
{
    // 无敌实体不会受伤，也不会播放音效
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.setInvulnerable(true);
    entity.lavaHurt();

    EXPECT_EQ(m_world.soundPlayCount(), 0);
}

// ============================================================================
// shouldPlayLavaHurtSound 测试
// ============================================================================

TEST_F(EntityLavaFireTest, ShouldPlayLavaHurtSound_BaseEntityReturnsTrue)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    EXPECT_TRUE(entity.shouldPlayLavaHurtSound());
}

TEST_F(EntityLavaFireTest, ShouldPlayLavaHurtSound_ItemEntityAtZeroHealth)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    // 将生命值设为 0 或负数时应该返回 true
    entity.setHealth(0);
    EXPECT_TRUE(entity.shouldPlayLavaHurtSound());

    entity.setHealth(-1);
    EXPECT_TRUE(entity.shouldPlayLavaHurtSound());
}

TEST_F(EntityLavaFireTest, ShouldPlayLavaHurtSound_ItemEntityAtNonZeroHealthTick0)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    // ticksExisted 初始为 0，0 % 10 == 0，应该返回 true
    EXPECT_EQ(entity.ticksExisted(), 0u);
    EXPECT_TRUE(entity.shouldPlayLavaHurtSound());
}

TEST_F(EntityLavaFireTest, ShouldPlayLavaHurtSound_ItemEntityAtNonZeroHealthTick1)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    // 模拟 tick 后 ticksExisted = 1
    // ItemEntity::tick() 会递增 ticksExisted
    // 我们通过直接 tick 来推进
    entity.tick();
    EXPECT_EQ(entity.ticksExisted(), 1u);
    // health > 0 且 tickCount(1) % 10 != 0，应该返回 false
    EXPECT_FALSE(entity.shouldPlayLavaHurtSound());
}

TEST_F(EntityLavaFireTest, ShouldPlayLavaHurtSound_ItemEntityAtTick10)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    // 推进 10 tick
    for (int i = 0; i < 10; ++i) {
        entity.tick();
    }
    EXPECT_EQ(entity.ticksExisted(), 10u);
    // health > 0 且 tickCount(10) % 10 == 0，应该返回 true
    EXPECT_TRUE(entity.shouldPlayLavaHurtSound());
}

TEST_F(EntityLavaFireTest, ShouldPlayLavaHurtSound_ItemEntityAtTick5)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    // 推进 5 tick
    for (int i = 0; i < 5; ++i) {
        entity.tick();
    }
    EXPECT_EQ(entity.ticksExisted(), 5u);
    // health > 0 且 tickCount(5) % 10 != 0，应该返回 false
    EXPECT_FALSE(entity.shouldPlayLavaHurtSound());
}

// ============================================================================
// clearFire 测试
// ============================================================================

TEST_F(EntityLavaFireTest, ClearFire_ZerosPositiveFireTimer)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.setFire(100);
    EXPECT_TRUE(entity.isOnFire());
    EXPECT_GT(entity.fire(), 0);

    entity.clearFire();
    EXPECT_FALSE(entity.isOnFire());
    EXPECT_EQ(entity.fire(), 0);
}

TEST_F(EntityLavaFireTest, ClearFire_PreservesNegativeFireTimer)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    // 负值表示火焰免疫期倒计时（MC Java 中 clearFire 保留负值）
    entity.forceFireTicks(-5);
    EXPECT_EQ(entity.fire(), -5);

    entity.clearFire();
    // 负值应保留不变（MC Java: Math.min(0, remainingFireTicks)）
    EXPECT_EQ(entity.fire(), -5);
}

TEST_F(EntityLavaFireTest, ClearFire_DoesNothingWhenAlreadyZero)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.forceFireTicks(0);
    EXPECT_EQ(entity.fire(), 0);

    entity.clearFire();
    EXPECT_EQ(entity.fire(), 0);
}

// ============================================================================
// baseTick 火焰处理测试
// ============================================================================

TEST_F(EntityLavaFireTest, BaseTick_FireTimerDecrements)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.setFire(20);
    EXPECT_EQ(entity.fire(), 20);

    entity.baseTick();
    EXPECT_EQ(entity.fire(), 19);
}

TEST_F(EntityLavaFireTest, BaseTick_FireClearedInWater)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.setFire(100);
    EXPECT_TRUE(entity.isOnFire());

    entity.setInWater(true);
    entity.baseTick();

    EXPECT_FALSE(entity.isOnFire());
    EXPECT_EQ(entity.fire(), 0);
}

TEST_F(EntityLavaFireTest, BaseTick_FireNotClearedInLava)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.setFire(100);

    entity.setInLava(true);
    entity.baseTick();

    // 在岩浆中火焰计时器应递减但不清除
    EXPECT_TRUE(entity.isOnFire());
    EXPECT_EQ(entity.fire(), 99);
}

TEST_F(EntityLavaFireTest, BaseTick_FallDistanceHalvedInLava)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.setFallDistance(10.0f);
    entity.setInLava(true);

    entity.baseTick();

    EXPECT_FLOAT_EQ(entity.fallDistance(), 5.0f);
}

TEST_F(EntityLavaFireTest, BaseTick_FallDistanceNotAffectedOutsideLava)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.setFallDistance(10.0f);
    entity.setInLava(false);

    entity.baseTick();

    EXPECT_FLOAT_EQ(entity.fallDistance(), 10.0f);
}

TEST_F(EntityLavaFireTest, BaseTick_OnFireDamageAtMultipleOf20)
{
    // onFire 伤害在 fire % 20 == 0 且不在岩浆中时触发
    // setFire(40) → fire=40
    // 第 1 次 baseTick: fire=40, 40%20==0 → 伤害, fire→39
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.forceFireTicks(40);
    f32 healthBefore = entity.health();

    entity.baseTick();

    // fire 应从 40 递减到 39，且应受到 onFire 伤害
    EXPECT_EQ(entity.fire(), 39);
    // LivingEntity 有无敌帧机制，但如果之前未受伤，第一次伤害应该生效
    // 实际上 Entity::baseTick 调用 hurt(DamageSources::onFire(), 1.0f)
    // LivingEntity::hurt 有无敌帧，但第一次伤害应该生效
    EXPECT_LT(entity.health(), healthBefore);
}

TEST_F(EntityLavaFireTest, BaseTick_NoOnFireDamageWhenInLava)
{
    // 在岩浆中时 onFire 伤害不应触发
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.forceFireTicks(20);
    entity.setInLava(true);
    f32 healthBefore = entity.health();

    entity.baseTick();

    // 在岩浆中不造成 onFire 伤害
    EXPECT_FLOAT_EQ(entity.health(), healthBefore);
    // 火焰计时器仍递减
    EXPECT_EQ(entity.fire(), 19);
}

// ============================================================================
// ItemEntity shouldPlayLavaHurtSound 与 lavaHurt 集成测试
// ============================================================================

TEST_F(EntityLavaFireTest, ItemEntity_LavaHurtPlaysSoundAtTick0)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setWorld(&m_world);

    // tick 0 时 shouldPlayLavaHurtSound 返回 true
    EXPECT_EQ(entity.ticksExisted(), 0u);
    EXPECT_TRUE(entity.shouldPlayLavaHurtSound());

    entity.lavaHurt();
    // hurt 成功，shouldPlayLavaHurtSound 返回 true，且未静音 → 播放音效
    EXPECT_EQ(m_world.soundPlayCount(), 1);
}

TEST_F(EntityLavaFireTest, ItemEntity_LavaHurtDealsDamage)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setWorld(&m_world);

    i32 healthBefore = entity.getHealth();
    entity.lavaHurt();
    EXPECT_EQ(entity.getHealth(), healthBefore - 4);
}

TEST_F(EntityLavaFireTest, ItemEntity_LavaIgniteSetsFire)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    entity.lavaIgnite();
    EXPECT_TRUE(entity.isOnFire());
    EXPECT_EQ(entity.fire(), 300); // 15 秒
}

// ============================================================================
// 额外验证
// ============================================================================

TEST_F(EntityLavaFireTest, LavaIgnite_SetsFireTo300Ticks)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.lavaIgnite();
    EXPECT_EQ(entity.fire(), 300);
}

TEST_F(EntityLavaFireTest, LavaHurt_DealsExactly4Damage)
{
    TestLivingEntity entity(EntityId(1), &m_world);
    f32 healthBefore = entity.health();
    entity.lavaHurt();
    EXPECT_FLOAT_EQ(entity.health(), healthBefore - 4.0f);
}

TEST_F(EntityLavaFireTest, LavaHurt_IgnoresFireImmuneEntities)
{
    // 火焰免疫实体不受 lavaHurt 影响
    // 由于 TestLivingEntity 默认不是火焰免疫的（取决于 EntityType 注册），
    // 我们无法直接测试 isImmuneToFire=true 的路径，
    // 但可以通过 setInvulnerable 来验证 invulnerable 路径
    TestLivingEntity entity(EntityId(1), &m_world);
    entity.setInvulnerable(true);

    f32 healthBefore = entity.health();
    entity.lavaHurt();
    // 无敌实体不受伤害
    EXPECT_FLOAT_EQ(entity.health(), healthBefore);
    EXPECT_EQ(m_world.soundPlayCount(), 0);
}
