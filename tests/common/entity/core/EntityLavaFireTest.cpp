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
#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/FireComponent.hpp"
#include "common/entity/ecs/systems/ticking/EnvironmentSensing.hpp"
#include "common/entity/ecs/systems/ticking/FireTick.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::item::tag;

// 测试辅助：驱动 fireTick free function（签名需 registry + view 两参，封装以便 13 处调用简洁）。
// fireTick 从 OOP baseTick 抽出，签名收 entt 原生 view 供 organizer 推导依赖。
namespace {
void runFireTick()
{
    auto& registry = mc::test::testEcsRegistry();
    mc::ecs::sys::fireTick(
        registry.raw(), registry.raw().view<mc::ecs::FireComponent, mc::ecs::EntityOwnerComponent>());
}

// 测试辅助：驱动 environmentSensing free function。
// B 阶段后 Entity::baseTick 不再内联刷新环境状态，改由 ecs::sys::environmentSensing
// （SystemPhase::EnvironmentSense，在 EntityTick 之前）每帧遍历碰撞箱内方块流体写
// EnvironmentStateComponent。测试不跑 EntityManager 调度，故需在 baseTick 前手动调一次，
// 模拟服务端"EnvironmentSense→EntityTick(baseTick)→PostEntityTick(fireTick)"同帧序列，
// 使 enableWaterAtEntity()/enableLavaAtEntity() 设的世界流体桩被 system 正确消费写组件。
// 安全性：Entity 析构经 registry.destroy 销毁 entt 实体（见 ~Entity），testEcsRegistry 中
// 不残留已析构实体，遍历无 UAF；world()==nullptr 的实体被 system 清零跳过。
void runEnvironmentSensing()
{
    auto& registry = mc::test::testEcsRegistry();
    mc::ecs::sys::environmentSensing(
        registry.raw(), registry.raw().view<mc::ecs::EnvironmentStateComponent, mc::ecs::EntityOwnerComponent>());
}
} // namespace

// ============================================================================
// 测试用 Mock World（支持 playSound 捕获）
// ============================================================================

class LavaFireTestWorld final : public mc::test::BaseTestWorld {
public:
    LavaFireTestWorld() = default;

    // 流体覆盖：测试可让世界在某坐标返回指定流体状态，
    // 这样 ecs::sys::environmentSensing（EnvironmentSense 阶段，baseTick 之前由测试经
    // runEnvironmentSensing() 手动触发）会遍历碰撞箱内方块流体写 EnvironmentStateComponent
    // 的 inWater/inLava，由世界流体驱动环境状态的语义（与 MC Java 由世界流体驱动 isInWater()
    // 一致）。默认 nullptr 表示该坐标无流体。直接返回 nullptr 而非 EMPTY 状态，
    // 使 environmentSensing 视该坐标为无流体。
    void setFluidOverride(const fluid::FluidState* state) { m_fluidOverride = state; }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        (void)x;
        (void)y;
        (void)z;
        return m_fluidOverride;
    }

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

    // 天气接口（用于雨中灭火测试）
    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    void setRaining(bool raining) { m_isRaining = raining; }

    [[nodiscard]] bool canRainAt(const BlockPos& pos) const override
    {
        MC_UNUSED(pos);
        return m_canRainAt;
    }
    void setCanRainAt(bool can) { m_canRainAt = can; }

    void clearState()
    {
        m_soundPlayCount = 0;
        m_lastSoundId = ResourceLocation();
        m_lastSoundVolume = 0.0f;
        m_lastSoundPitch = 0.0f;
        m_gameEventCount = 0;
        m_lastGameEventId.clear();
        m_lastGameEventPos = BlockPos(0, 0, 0);
        m_fluidOverride = nullptr;
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
    bool m_isRaining = false;
    bool m_canRainAt = false;
    const fluid::FluidState* m_fluidOverride = nullptr;
};

// ============================================================================
// 测试用 LivingEntity 子类（用于需要 hurt 逻辑的测试）
// ============================================================================

namespace {

class TestLivingEntity : public LivingEntity {
public:
    explicit TestLivingEntity(
        EntityInstanceId id, IWorld* world = nullptr, ecs::EntityRegistry& registry = mc::test::testEcsRegistry())
        : LivingEntity(id, world, registry)
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

class TestPlayer : public Player {
public:
    explicit TestPlayer(IWorld* world)
        : Player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
        if (world != nullptr) {
            setWorld(world);
        }
    }

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

    // 让测试世界在实体所在坐标返回水源流体状态。
    // 测试在 baseTick 前调 runEnvironmentSensing()，environmentSensing 据此把
    // EnvironmentStateComponent.inWater 置 true，使后续火焰处理走"水中灭火"分支——
    // 这与 MC Java 由世界流体驱动 isInWater() 的语义一致。
    // （B 阶段前用 Entity::setInWater(true)，但 baseTick 内联 updateEnvironmentState 会重置它；
    //  B 阶段后 baseTick 不再刷新环境状态，改由 system 消费世界流体桩，setInWater setter 已删。）
    void enableWaterAtEntity()
    {
        auto* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        ASSERT_NE(waterFluid, nullptr);
        m_world.setFluidOverride(&waterFluid->defaultState());
    }

    // 让测试世界在实体所在坐标返回岩浆源流体状态（语义同 enableWaterAtEntity）。
    void enableLavaAtEntity()
    {
        auto* lavaFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::LAVA_ID);
        ASSERT_NE(lavaFluid, nullptr);
        m_world.setFluidOverride(&lavaFluid->defaultState());
    }

    LavaFireTestWorld m_world;
};

// ============================================================================
// lavaIgnite 测试
// ============================================================================

TEST_F(EntityLavaFireTest, LavaIgnite_SetsFireForNonImmuneEntity)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    EXPECT_FALSE(entity.isOnFire());

    entity.lavaIgnite();
    EXPECT_TRUE(entity.isOnFire());
    EXPECT_EQ(entity.fire(), 300); // 15 秒 = 300 ticks
}

TEST_F(EntityLavaFireTest, LavaIgnite_DoesNotReduceExistingHigherFireTime)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    // 设置一个很高的火焰时间
    entity.forceFireTicks(500);
    entity.lavaIgnite();
    // lavaIgnite 内部调用 setFire(300)，但 setFire 只增加不减少
    EXPECT_EQ(entity.fire(), 500);
}

TEST_F(EntityLavaFireTest, LavaIgnite_IncreasesLowFireTime)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
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
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    f32 healthBefore = entity.health();

    entity.lavaHurt();
    EXPECT_LT(entity.health(), healthBefore);
    EXPECT_EQ(entity.health(), healthBefore - 4.0f);
}

TEST_F(EntityLavaFireTest, LavaHurt_PlaysSoundWhenDamageSucceeds)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
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
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.setSilent(true);
    entity.lavaHurt();

    // 静音实体不应播放音效
    EXPECT_EQ(m_world.soundPlayCount(), 0);
}

TEST_F(EntityLavaFireTest, LavaHurt_NoSoundWhenDamageFails)
{
    // 无敌实体不会受伤，也不会播放音效
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.setInvulnerable(true);
    entity.lavaHurt();

    EXPECT_EQ(m_world.soundPlayCount(), 0);
}

// ============================================================================
// shouldPlayLavaHurtSound 测试
// ============================================================================

TEST_F(EntityLavaFireTest, ShouldPlayLavaHurtSound_BaseEntityReturnsTrue)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    EXPECT_TRUE(entity.shouldPlayLavaHurtSound());
}

TEST_F(EntityLavaFireTest, ShouldPlayLavaHurtSound_ItemEntityAtZeroHealth)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityInstanceId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

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
    ItemEntity entity(EntityInstanceId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // ticksExisted 初始为 0，0 % 10 == 0，应该返回 true
    EXPECT_EQ(entity.ticksExisted(), 0u);
    EXPECT_TRUE(entity.shouldPlayLavaHurtSound());
}

TEST_F(EntityLavaFireTest, ShouldPlayLavaHurtSound_ItemEntityAtNonZeroHealthTick1)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityInstanceId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

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
    ItemEntity entity(EntityInstanceId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

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
    ItemEntity entity(EntityInstanceId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

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
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.setFire(100);
    EXPECT_TRUE(entity.isOnFire());
    EXPECT_GT(entity.fire(), 0);

    entity.clearFire();
    EXPECT_FALSE(entity.isOnFire());
    EXPECT_EQ(entity.fire(), 0);
}

TEST_F(EntityLavaFireTest, ClearFire_PreservesNegativeFireTimer)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    // 负值表示火焰免疫期倒计时（MC Java 中 clearFire 保留负值）
    entity.forceFireTicks(-5);
    EXPECT_EQ(entity.fire(), -5);

    entity.clearFire();
    // 负值应保留不变（MC Java: Math.min(0, remainingFireTicks)）
    EXPECT_EQ(entity.fire(), -5);
}

TEST_F(EntityLavaFireTest, ClearFire_DoesNothingWhenAlreadyZero)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
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
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.setFire(20);
    EXPECT_EQ(entity.fire(), 20);

    entity.baseTick();
    runFireTick();
    EXPECT_EQ(entity.fire(), 19);
}

TEST_F(EntityLavaFireTest, BaseTick_FireClearedInWater)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.setFire(100);
    EXPECT_TRUE(entity.isOnFire());

    // 通过世界流体让 environmentSensing 把 EnvironmentStateComponent.inWater 置 true
    // （baseTick 已不再内联刷新环境状态，改由 environmentSensing system 每帧重写组件）。
    enableWaterAtEntity();
    runEnvironmentSensing();
    entity.baseTick();
    runFireTick();

    EXPECT_FALSE(entity.isOnFire());
    EXPECT_EQ(entity.fire(), 0);
}

TEST_F(EntityLavaFireTest, BaseTick_FireNotClearedInLava)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.setFire(100);

    // 用世界流体驱动 isInLava()：environmentSensing 消费流体桩写组件。
    enableLavaAtEntity();
    runEnvironmentSensing();
    entity.baseTick();
    runFireTick();

    // 在岩浆中火焰计时器应递减但不清除
    EXPECT_TRUE(entity.isOnFire());
    EXPECT_EQ(entity.fire(), 99);
}

TEST_F(EntityLavaFireTest, BaseTick_FallDistanceHalvedInLava)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.setFallDistance(10.0f);
    // 用世界流体驱动 isInLava()：environmentSensing 消费流体桩写组件。
    enableLavaAtEntity();
    runEnvironmentSensing();

    entity.baseTick();
    runFireTick();

    EXPECT_FLOAT_EQ(entity.fallDistance(), 5.0f);
}

TEST_F(EntityLavaFireTest, BaseTick_FallDistanceNotAffectedOutsideLava)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.setFallDistance(10.0f);
    // 不在岩浆中：不启用流体覆盖。EnvironmentStateComponent.inLava 由
    // ecs::sys::environmentSensing 每帧刷新，本测试直接置 false 模拟"不在岩浆中"。
    test::setEntityInLava(entity, false);

    entity.baseTick();
    runFireTick();

    EXPECT_FLOAT_EQ(entity.fallDistance(), 10.0f);
}

TEST_F(EntityLavaFireTest, BaseTick_OnFireDamageAtMultipleOf20)
{
    // onFire 伤害在 fire % 20 == 0 且不在岩浆中时触发
    // setFire(40) → fire=40
    // 第 1 次 baseTick: fire=40, 40%20==0 → 伤害, fire→39
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.forceFireTicks(40);
    f32 healthBefore = entity.health();

    entity.baseTick();
    runFireTick();

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
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.forceFireTicks(20);
    // 用世界流体驱动 isInLava()：environmentSensing 消费流体桩写组件。
    enableLavaAtEntity();
    runEnvironmentSensing();
    f32 healthBefore = entity.health();

    entity.baseTick();
    runFireTick();

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
    ItemEntity entity(EntityInstanceId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());
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
    ItemEntity entity(EntityInstanceId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());
    entity.setWorld(&m_world);

    i32 healthBefore = entity.getHealth();
    entity.lavaHurt();
    EXPECT_EQ(entity.getHealth(), healthBefore - 4);
}

TEST_F(EntityLavaFireTest, ItemEntity_LavaIgniteSetsFire)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityInstanceId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    entity.lavaIgnite();
    EXPECT_TRUE(entity.isOnFire());
    EXPECT_EQ(entity.fire(), 300); // 15 秒
}

// ============================================================================
// 额外验证
// ============================================================================

TEST_F(EntityLavaFireTest, LavaIgnite_SetsFireTo300Ticks)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.lavaIgnite();
    EXPECT_EQ(entity.fire(), 300);
}

TEST_F(EntityLavaFireTest, LavaHurt_DealsExactly4Damage)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
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
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.setInvulnerable(true);

    f32 healthBefore = entity.health();
    entity.lavaHurt();
    // 无敌实体不受伤害
    EXPECT_FLOAT_EQ(entity.health(), healthBefore);
    EXPECT_EQ(m_world.soundPlayCount(), 0);
}

// ============================================================================
// 火焰免疫期（fireImmuneTicks）机制测试
// ============================================================================

TEST_F(EntityLavaFireTest, GetFireImmuneTicks_BaseEntityReturnsZero)
{
    // 基类 Entity 返回 0（无免疫期）
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    EXPECT_EQ(entity.getFireImmuneTicks(), 0);
}

TEST_F(EntityLavaFireTest, GetRemainingFireTicks_PositiveWhenBurning)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.igniteForSeconds(5.0f);
    EXPECT_GT(entity.getRemainingFireTicks(), 0);
    EXPECT_EQ(entity.getRemainingFireTicks(), 100); // 5秒 = 100 ticks
}

TEST_F(EntityLavaFireTest, GetRemainingFireTicks_NegativeWhenImmune)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.forceFireTicks(-10);
    EXPECT_LT(entity.getRemainingFireTicks(), 0);
    EXPECT_EQ(entity.getRemainingFireTicks(), -10);
}

TEST_F(EntityLavaFireTest, IgniteForSeconds_SetsCorrectTicks)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.igniteForSeconds(8.0f);
    EXPECT_EQ(entity.getRemainingFireTicks(), 160); // 8秒 = 160 ticks
}

TEST_F(EntityLavaFireTest, IgniteForSeconds_DoesNotReduceExistingHigherFireTime)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.igniteForSeconds(15.0f); // 300 ticks
    entity.igniteForSeconds(5.0f);  // 100 ticks < 300，不更新
    EXPECT_EQ(entity.getRemainingFireTicks(), 300);
}

TEST_F(EntityLavaFireTest, IgniteForSeconds_OverwritesImmunityCooldown)
{
    // 免疫期（负值）时，igniteForSeconds 应覆盖免疫期
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.forceFireTicks(-20); // 免疫期
    EXPECT_LT(entity.getRemainingFireTicks(), 0);

    entity.igniteForSeconds(8.0f); // 160 ticks > -20，覆盖
    EXPECT_EQ(entity.getRemainingFireTicks(), 160);
    EXPECT_TRUE(entity.isOnFire());
}

TEST_F(EntityLavaFireTest, IgniteForTicks_SetsCorrectTicks)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.igniteForTicks(50);
    EXPECT_EQ(entity.getRemainingFireTicks(), 50);
}

TEST_F(EntityLavaFireTest, SetRemainingFireTicks_DirectlySetsValue)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.setRemainingFireTicks(-15);
    EXPECT_EQ(entity.getRemainingFireTicks(), -15);

    entity.setRemainingFireTicks(200);
    EXPECT_EQ(entity.getRemainingFireTicks(), 200);
}

TEST_F(EntityLavaFireTest, IsOnFire_FalseWhenImmuneCooldown)
{
    // 负值免疫期时，isOnFire() 应返回 false
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.forceFireTicks(-10);
    EXPECT_FALSE(entity.isOnFire());
}

TEST_F(EntityLavaFireTest, FireImmunityCooldown_SetByWaterExtinguish)
{
    // 在水中灭火时，如果 getFireImmuneTicks() > 0 则设置免疫期
    // TestLivingEntity 的 getFireImmuneTicks() 返回 0，所以不会设置免疫期
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.igniteForSeconds(5.0f);
    EXPECT_TRUE(entity.isOnFire());

    // 用世界流体驱动 isInWater()：environmentSensing 消费流体桩写组件。
    enableWaterAtEntity();
    runEnvironmentSensing();
    entity.baseTick();
    runFireTick();

    // 基类 getFireImmuneTicks() 返回 0，所以 m_fire 应为 0 而非负值
    EXPECT_EQ(entity.getRemainingFireTicks(), 0);
    EXPECT_FALSE(entity.isOnFire());
}

TEST_F(EntityLavaFireTest, FireImmunityCooldown_SetByWaterExtinguishWithPlayer)
{
    // Player 的 getFireImmuneTicks() 返回 20，水中灭火应设置免疫期 -20
    TestPlayer player(&m_world);
    player.igniteForSeconds(5.0f);
    EXPECT_TRUE(player.isOnFire());

    // 用世界流体驱动 isInWater()：environmentSensing 消费流体桩写组件。
    enableWaterAtEntity();
    runEnvironmentSensing();
    player.baseTick();
    runFireTick();

    // Player 应获得 20 tick 免疫期
    EXPECT_EQ(player.getRemainingFireTicks(), -20);
    EXPECT_FALSE(player.isOnFire());
}

TEST_F(EntityLavaFireTest, FireImmunityCooldown_PlayerCreativeModeFireLimit)
{
    // 创造模式下 forceFireTicks 限制为 max 1 tick
    TestPlayer player(&m_world);
    player.abilities().invulnerable = true;

    player.forceFireTicks(300);
    EXPECT_EQ(player.getRemainingFireTicks(), 1); // 创造模式限制

    player.abilities().invulnerable = false;
    player.forceFireTicks(300);
    EXPECT_EQ(player.getRemainingFireTicks(), 300); // 非创造模式无限制
}

TEST_F(EntityLavaFireTest, FireImmunityCooldown_PlayerImmuneTicksIs20)
{
    TestPlayer player(&m_world);
    EXPECT_EQ(player.getFireImmuneTicks(), 20);
}

TEST_F(EntityLavaFireTest, ExtinguishFire_PlaysSoundWhenBurning)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.igniteForSeconds(5.0f);
    EXPECT_TRUE(entity.isOnFire());

    entity.extinguishFire();
    EXPECT_FALSE(entity.isOnFire());
    EXPECT_EQ(m_world.soundPlayCount(), 1);
    EXPECT_EQ(m_world.lastSoundId(), SoundEvents::ENTITY_GENERIC_EXTINGUISH_FIRE);
}

TEST_F(EntityLavaFireTest, ExtinguishFire_NoSoundWhenNotBurning)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    EXPECT_FALSE(entity.isOnFire());

    entity.extinguishFire();
    EXPECT_EQ(m_world.soundPlayCount(), 0);
}

TEST_F(EntityLavaFireTest, PlayExtinguishSound_PlaysCorrectSound)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.playExtinguishSound();
    EXPECT_EQ(m_world.soundPlayCount(), 1);
    EXPECT_EQ(m_world.lastSoundId(), SoundEvents::ENTITY_GENERIC_EXTINGUISH_FIRE);
    EXPECT_FLOAT_EQ(m_world.lastSoundVolume(), 0.7f);
}

TEST_F(EntityLavaFireTest, ClearFire_NegativeValuePreserved)
{
    // clearFire 保留负值免疫期
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.forceFireTicks(-15);
    entity.clearFire();
    EXPECT_EQ(entity.getRemainingFireTicks(), -15); // 负值不变
}

TEST_F(EntityLavaFireTest, ClearFire_PositiveValueZeroed)
{
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    entity.igniteForSeconds(5.0f);
    EXPECT_GT(entity.getRemainingFireTicks(), 0);
    entity.clearFire();
    EXPECT_EQ(entity.getRemainingFireTicks(), 0);
}

TEST_F(EntityLavaFireTest, DoBlockCollisions_SetsImmunityWhenNoFire)
{
    // 当实体不燃烧且方块碰撞未点燃时，应设置免疫期
    // 使用 Player 测试，因为基类 getFireImmuneTicks() 返回 0
    TestPlayer player(&m_world);
    EXPECT_FALSE(player.isOnFire());
    EXPECT_EQ(player.getRemainingFireTicks(), 0);

    player.doBlockCollisions();

    // 不在火方块中，不燃烧，应设置免疫期
    EXPECT_EQ(player.getRemainingFireTicks(), -20); // Player 免疫期 = 20
}

TEST_F(EntityLavaFireTest, DoBlockCollisions_NoImmunityWhenFireIncreased)
{
    // 当方块碰撞增加了火焰时间，不应设置免疫期
    TestPlayer player(&m_world);
    player.igniteForSeconds(5.0f); // 先点燃
    EXPECT_TRUE(player.isOnFire());

    player.doBlockCollisions();

    // 已在燃烧，不应设置免疫期
    EXPECT_GT(player.getRemainingFireTicks(), 0);
}

// ============================================================================
// 灭火音效测试（水中/雨中灭火应播放音效）
// ============================================================================

TEST_F(EntityLavaFireTest, WaterExtinguish_PlaysSound)
{
    // MC Java: 水中灭火应播放 GENERIC_EXTINGUISH_FIRE 音效
    // 现在水中灭火使用 extinguishFire()，包含音效
    TestPlayer player(&m_world);
    player.igniteForSeconds(5.0f);
    EXPECT_TRUE(player.isOnFire());

    // 用世界流体驱动 isInWater()：environmentSensing 消费流体桩写组件。
    enableWaterAtEntity();
    runEnvironmentSensing();
    player.baseTick();
    runFireTick();

    EXPECT_FALSE(player.isOnFire());
    EXPECT_EQ(m_world.soundPlayCount(), 1);
    EXPECT_EQ(m_world.lastSoundId(), SoundEvents::ENTITY_GENERIC_EXTINGUISH_FIRE);
}

TEST_F(EntityLavaFireTest, WaterExtinguish_NoSoundWhenNotBurning)
{
    // 不在燃烧时进入水中，不应播放灭火音效
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    EXPECT_FALSE(entity.isOnFire());

    // 用世界流体驱动 isInWater()：environmentSensing 消费流体桩写组件。
    enableWaterAtEntity();
    runEnvironmentSensing();
    entity.baseTick();
    runFireTick();

    EXPECT_EQ(m_world.soundPlayCount(), 0);
}

TEST_F(EntityLavaFireTest, RainExtinguish_PlaysSound)
{
    // MC Java: 雨中灭火应播放 GENERIC_EXTINGUISH_FIRE 音效
    // 使用不可被 20 整除的火焰时间，避免同一 tick 触发 onFire 伤害音效干扰
    TestPlayer player(&m_world);
    player.igniteForSeconds(5.0f);
    // 设置火焰为非 20 整数值，避免 baseTick 中 onFire 伤害播放受伤音效
    player.forceFireTicks(99);
    EXPECT_TRUE(player.isOnFire());

    m_world.setRaining(true);
    m_world.setCanRainAt(true);
    player.baseTick();
    runFireTick();

    EXPECT_FALSE(player.isOnFire());
    EXPECT_EQ(m_world.soundPlayCount(), 1);
    EXPECT_EQ(m_world.lastSoundId(), SoundEvents::ENTITY_GENERIC_EXTINGUISH_FIRE);
}

TEST_F(EntityLavaFireTest, RainExtinguish_NoSoundWhenNotBurning)
{
    // 不在燃烧时在雨中，不应播放灭火音效
    TestLivingEntity entity(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    EXPECT_FALSE(entity.isOnFire());

    m_world.setRaining(true);
    m_world.setCanRainAt(true);
    entity.baseTick();
    runFireTick();

    EXPECT_EQ(m_world.soundPlayCount(), 0);
}
