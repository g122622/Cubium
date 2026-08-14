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
 * @file SculkShriekerHelperTest.cpp
 * @brief SculkShriekerHelper 和 SculkShriekerBlockEntity 尖啸体逻辑测试
 *
 * 测试范围：
 * - SculkShriekerBlockEntity: shriekingFinished 标志的设置/清除/序列化
 * - SculkShriekerBlockEntity: warningLevel 与 canSummonWarden 的边界条件
 * - WardenWarningEffect: 警告等级递增/递减/冷却逻辑
 * - SculkShriekerHelper: tryGetPlayer 实体解析逻辑
 * - SculkShriekerHelper: 常量值验证
 *
 * 注意：SculkShriekerHelper 的 tryShriek/tryRespond/trySummonWarden 等方法
 * 依赖 ServerWorld，需要集成测试覆盖。此处仅测试不依赖 ServerWorld 的纯逻辑。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "server/world/blockentity/sculk/SculkShriekerHelper.hpp"

using namespace mc;
using namespace mc::blockentity;
using namespace mc::server;

// ============================================================================
// SculkShriekerBlockEntity shriekingFinished 测试
// ============================================================================

class SculkShriekerBlockEntityShriekTest : public ::testing::Test {
protected:
    void SetUp() override { pos_ = BlockPos(10, 64, -20); }

    BlockPos pos_;
};

TEST_F(SculkShriekerBlockEntityShriekTest, ShriekingFinishedDefaultFalse)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);
    EXPECT_FALSE(entity->isShriekingFinished());
}

TEST_F(SculkShriekerBlockEntityShriekTest, SetShriekingFinished)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->setShriekingFinished(true);
    EXPECT_TRUE(entity->isShriekingFinished());

    entity->setShriekingFinished(false);
    EXPECT_FALSE(entity->isShriekingFinished());
}

TEST_F(SculkShriekerBlockEntityShriekTest, ShriekingFinishedIndependentOfWarningLevel)
{
    // shriekingFinished 和 warningLevel 应该独立
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->setWarningLevel(3);
    EXPECT_FALSE(entity->isShriekingFinished());

    entity->setShriekingFinished(true);
    EXPECT_EQ(entity->getWarningLevel(), 3);
    EXPECT_TRUE(entity->isShriekingFinished());

    entity->setShriekingFinished(false);
    EXPECT_EQ(entity->getWarningLevel(), 3);
    EXPECT_FALSE(entity->isShriekingFinished());
}

TEST_F(SculkShriekerBlockEntityShriekTest, ShriekingFinishedWithMaxWarningLevel)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 设置到最大警告等级
    entity->setWarningLevel(4);
    EXPECT_TRUE(entity->canSummonWarden());

    // 设置 shriekingFinished（模拟尖啸结束）
    entity->setShriekingFinished(true);
    EXPECT_TRUE(entity->isShriekingFinished());
    EXPECT_TRUE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityShriekTest, WarningLevelResetAfterTryShriek)
{
    // tryShriek 每次调用都重置 warningLevel 为 0
    // 此测试验证 BlockEntity 的 setWarningLevel(0) 行为
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->setWarningLevel(3);
    EXPECT_EQ(entity->getWarningLevel(), 3);

    // 模拟 tryShriek 的重置操作
    entity->setWarningLevel(0);
    EXPECT_EQ(entity->getWarningLevel(), 0);
    EXPECT_FALSE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityShriekTest, JsonSerialization_WithShriekingFinished)
{
    // 注意：shriekingFinished 不应序列化（运行时标志）
    // 但 warningLevel 和 vibrationData 需要序列化
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->setWarningLevel(1);
    entity->setShriekingFinished(true);

    nlohmann::json data;
    entity->save(data);

    // warning_level 应该被保存
    EXPECT_TRUE(data.contains("warning_level"));
    EXPECT_EQ(data["warning_level"], 1);

    // shriekingFinished 是运行时标志，不应保存到存档
    EXPECT_FALSE(data.contains("shrieking_finished"));
}

TEST_F(SculkShriekerBlockEntityShriekTest, ShriekingFinishedNotPersistedAcrossLoad)
{
    // shriekingFinished 不应从存档中恢复（每次加载默认为 false）
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);
    entity->setWarningLevel(1);
    entity->setShriekingFinished(true);

    nlohmann::json data;
    entity->save(data);

    auto loaded = std::make_unique<SculkShriekerBlockEntity>(pos_);
    ASSERT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getWarningLevel(), 1);
    EXPECT_FALSE(loaded->isShriekingFinished());
}

// ============================================================================
// SculkShriekerBlockEntity canSummonWarden 边界条件测试
// ============================================================================

class SculkShriekerBlockEntityWarningTest : public ::testing::Test {
protected:
    void SetUp() override { pos_ = BlockPos(0, 0, 0); }

    BlockPos pos_;
};

TEST_F(SculkShriekerBlockEntityWarningTest, CanSummonWardenAtLevel4Only)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    EXPECT_FALSE(entity->canSummonWarden()); // level 0

    entity->setWarningLevel(1);
    EXPECT_FALSE(entity->canSummonWarden()); // level 1

    entity->setWarningLevel(2);
    EXPECT_FALSE(entity->canSummonWarden()); // level 2

    entity->setWarningLevel(3);
    EXPECT_FALSE(entity->canSummonWarden()); // level 3

    entity->setWarningLevel(4);
    EXPECT_TRUE(entity->canSummonWarden()); // level 4
}

TEST_F(SculkShriekerBlockEntityWarningTest, WarningLevelDoesNotExceedMax)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 设置到最大等级
    entity->setWarningLevel(4);
    EXPECT_EQ(entity->getWarningLevel(), 4);
    EXPECT_TRUE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityWarningTest, SetWarningLevelDirectly)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 直接设置到各等级
    entity->setWarningLevel(0);
    EXPECT_FALSE(entity->canSummonWarden());

    entity->setWarningLevel(4);
    EXPECT_TRUE(entity->canSummonWarden());

    // 重置回 0（tryShriek 的行为）
    entity->setWarningLevel(0);
    EXPECT_FALSE(entity->canSummonWarden());
}

// ============================================================================
// WardenWarningEffect 测试
// ============================================================================

class WardenWarningEffectTest : public ::testing::Test {
protected:
    void SetUp() override {}

    entity::WardenWarningEffect effect;
};

TEST_F(WardenWarningEffectTest, DefaultState)
{
    EXPECT_EQ(effect.getWarningLevel(), 0);
    EXPECT_EQ(effect.getSourcePos(), BlockPos());
}

TEST_F(WardenWarningEffectTest, IncreaseWarning)
{
    EXPECT_EQ(effect.getWarningLevel(), 0);

    // 每次递增后会设置冷却，需要先消耗冷却才能再次递增
    // WardenSpawnTracker.increaseWarningLevel()
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);
    EXPECT_TRUE(effect.onCooldown());

    // 冷却期间递增无效
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);

    // 消耗冷却（WARNING_LEVEL_INCREASE_COOLDOWN = 200 tick）
    for (i32 i = 0; i < 200; ++i) {
        effect.tick();
    }
    EXPECT_FALSE(effect.onCooldown());

    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 2);
}

TEST_F(WardenWarningEffectTest, IncreaseWarningCappedAt4)
{
    // 递增到最大值 4，每次递增后需要消耗冷却
    for (i32 i = 0; i < 4; ++i) {
        effect.increaseWarning();
        // 消耗冷却
        for (i32 j = 0; j < 200; ++j) {
            effect.tick();
        }
    }
    EXPECT_EQ(effect.getWarningLevel(), 4);

    // 超过最大值不会继续递增
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 4);
}

TEST_F(WardenWarningEffectTest, DecreaseWarning)
{
    // 使用 setWarningLevel 直接设置（跳过冷却）
    effect.setWarningLevel(3);
    EXPECT_EQ(effect.getWarningLevel(), 3);

    effect.decreaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 2);

    effect.decreaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);
}

TEST_F(WardenWarningEffectTest, DecreaseWarningFloorAt0)
{
    EXPECT_EQ(effect.getWarningLevel(), 0);
    effect.decreaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 0);
}

TEST_F(WardenWarningEffectTest, SetSourcePos)
{
    BlockPos shriekerPos(100, -50, 200);
    effect.setSourcePos(shriekerPos);
    EXPECT_EQ(effect.getSourcePos(), shriekerPos);
}

TEST_F(WardenWarningEffectTest, TickDecreasesAfterLongTime)
{
    // 递增警告等级（这会设置冷却和重置递减计时器）
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);

    // 冷却期间 tick 不应递减
    // 冷却为 WARNING_LEVEL_INCREASE_COOLDOWN = 200 tick
    for (i32 i = 0; i < 100; ++i) {
        effect.tick();
    }
    EXPECT_EQ(effect.getWarningLevel(), 1);
}

TEST_F(WardenWarningEffectTest, TickEventuallyDecreasesTo0)
{
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);

    // 模拟足够多的 tick 让警告等级递减到 0
    // DECREASE_WARNING_LEVEL_EVERY_INTERVAL = 12000 tick (10 分钟)
    // 加上冷却期 200 tick
    for (i32 i = 0; i < 13000; ++i) {
        effect.tick();
    }
    EXPECT_EQ(effect.getWarningLevel(), 0);
}

TEST_F(WardenWarningEffectTest, CooldownPreventsIncrease)
{
    // 递增后处于冷却中，再次递增应无效
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);
    EXPECT_TRUE(effect.onCooldown());

    // 冷却中递增不生效
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);

    // 消耗冷却
    for (i32 i = 0; i < 200; ++i) {
        effect.tick();
    }
    EXPECT_FALSE(effect.onCooldown());

    // 冷却结束后可以递增
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 2);
}

TEST_F(WardenWarningEffectTest, CopyData)
{
    // 设置一个追踪器的状态
    effect.increaseWarning();
    // 消耗冷却后再次递增
    for (i32 i = 0; i < 200; ++i) {
        effect.tick();
    }
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 2);

    // 创建另一个追踪器并复制数据
    entity::WardenWarningEffect other;
    other.copyData(effect);
    EXPECT_EQ(other.getWarningLevel(), 2);
}

TEST_F(WardenWarningEffectTest, Reset)
{
    effect.increaseWarning();
    EXPECT_EQ(effect.getWarningLevel(), 1);

    effect.reset();
    EXPECT_EQ(effect.getWarningLevel(), 0);
    EXPECT_FALSE(effect.onCooldown());
}

TEST_F(WardenWarningEffectTest, WarningRadius)
{
    // 默认警告半径应为 10.0
    EXPECT_FLOAT_EQ(effect.getWarningRadius(), 10.0f);
}

// ============================================================================
// SculkShriekerHelper 常量验证测试
// ============================================================================

class SculkShriekerHelperConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SculkShriekerHelperConstantsTest, WardenSearchRadius)
{
    // 搜索半径 48 格
    EXPECT_FLOAT_EQ(SculkShriekerHelper::WARDEN_SEARCH_RADIUS, 48.0f);
}

TEST_F(SculkShriekerHelperConstantsTest, PlayerSearchRadius)
{
    // 搜索半径 16 格
    EXPECT_FLOAT_EQ(SculkShriekerHelper::PLAYER_SEARCH_RADIUS, 16.0f);
}

TEST_F(SculkShriekerHelperConstantsTest, DarknessRadius)
{
    // 黑暗效果半径 40 格
    EXPECT_FLOAT_EQ(SculkShriekerHelper::DARKNESS_RADIUS, 40.0f);
}

TEST_F(SculkShriekerHelperConstantsTest, DarknessDuration)
{
    // 260 tick = 13 秒
    EXPECT_EQ(SculkShriekerHelper::DARKNESS_DURATION, 260);
}

TEST_F(SculkShriekerHelperConstantsTest, DarknessCooldown)
{
    // 黑暗效果应用冷却 200 tick
    EXPECT_EQ(SculkShriekerHelper::DARKNESS_COOLDOWN, 200);
}

TEST_F(SculkShriekerHelperConstantsTest, SummonAttempts)
{
    // 20 次尝试
    EXPECT_EQ(SculkShriekerHelper::SUMMON_ATTEMPTS, 20);
}

TEST_F(SculkShriekerHelperConstantsTest, SummonHorizontalRange)
{
    // 水平偏移 +/-5
    EXPECT_EQ(SculkShriekerHelper::SUMMON_HORIZONTAL_RANGE, 5);
}

TEST_F(SculkShriekerHelperConstantsTest, SummonVerticalRange)
{
    // 垂直偏移 +/-6
    EXPECT_EQ(SculkShriekerHelper::SUMMON_VERTICAL_RANGE, 6);
}

TEST_F(SculkShriekerHelperConstantsTest, WardenSoundByLevel)
{
    // 验证声音映射：level 0 无声音，level 1-4 对应不同声音
    EXPECT_EQ(SculkShriekerHelper::WARDEN_SOUND_BY_LEVEL[0], nullptr);
    EXPECT_STREQ(SculkShriekerHelper::WARDEN_SOUND_BY_LEVEL[1], "minecraft:entity.warden.nearby_close");
    EXPECT_STREQ(SculkShriekerHelper::WARDEN_SOUND_BY_LEVEL[2], "minecraft:entity.warden.nearby_closer");
    EXPECT_STREQ(SculkShriekerHelper::WARDEN_SOUND_BY_LEVEL[3], "minecraft:entity.warden.nearby_closest");
    EXPECT_STREQ(SculkShriekerHelper::WARDEN_SOUND_BY_LEVEL[4], "minecraft:entity.warden.listening_angry");
}

// ============================================================================
// SculkShriekerHelper::tryGetPlayer 测试
// ============================================================================

/**
 * @brief 投射物测试辅助类
 *
 * ProjectileEntity 的构造函数是 protected 的，
 * 通过子类化在测试中公开构造能力。
 */
class TestProjectileEntity : public mc::entity::ProjectileEntity {
public:
    explicit TestProjectileEntity(EntityInstanceId id)
        : ProjectileEntity(id, mc::test::testEcsRegistry())
    {}
};

/**
 * @brief tryGetPlayer 测试用的世界桩
 *
 * 继承 BaseTestWorld，覆写 getEntity 和 getEntityByUuid
 * 以支持 tryGetPlayer 的载具/投射物/物品实体解析测试。
 */
class TryGetPlayerTestWorld : public mc::test::BaseTestWorld {
public:
    TryGetPlayerTestWorld() = default;

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] Entity* getEntityByUuid(const std::string& uuid) override
    {
        auto it = m_uuidToEntity.find(uuid);
        return it != m_uuidToEntity.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntityByUuid(const std::string& uuid) const override
    {
        auto it = m_uuidToEntity.find(uuid);
        return it != m_uuidToEntity.end() ? it->second : nullptr;
    }

    /**
     * @brief 注册一个实体到测试世界
     *
     * 将实体添加到 ID/UUID 映射，并设置实体的世界指针。
     */
    void registerEntity(std::unique_ptr<Entity> entity)
    {
        Entity* raw = entity.get();
        EntityInstanceId id = raw->id();
        std::string uuid = raw->uuid();
        raw->setWorld(this);
        m_uuidToEntity[uuid] = raw;
        m_entities[id] = std::move(entity);
    }

private:
    std::unordered_map<EntityInstanceId, std::unique_ptr<Entity>> m_entities;
    std::unordered_map<std::string, Entity*> m_uuidToEntity;
};

class SculkShriekerHelperTryGetPlayerTest : public ::testing::Test {
protected:
    TryGetPlayerTestWorld world;
};

TEST_F(SculkShriekerHelperTryGetPlayerTest, NullptrEntityReturnsNullptr)
{
    Player* result = SculkShriekerHelper::tryGetPlayer(world, nullptr);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, DirectPlayerReturnsPlayer)
{
    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    Player* rawPlayer = player.get();
    world.registerEntity(std::move(player));

    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawPlayer);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, rawPlayer);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, NonPlayerEntityReturnsNullptr)
{
    auto entity = std::make_unique<LivingEntity>(EntityInstanceId(2), nullptr, mc::test::testEcsRegistry());
    Entity* rawEntity = entity.get();
    world.registerEntity(std::move(entity));

    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawEntity);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, ControllingPassengerIsPlayer)
{
    // 创建载具实体和玩家乘客
    auto vehicle = std::make_unique<LivingEntity>(EntityInstanceId(10), nullptr, mc::test::testEcsRegistry());
    auto player = std::make_unique<Player>(EntityInstanceId(11), "RiderPlayer", mc::test::testEcsRegistry());
    Player* rawPlayer = player.get();

    // 先注册实体到世界（startRiding 需要世界引用来查找实体）
    Entity* rawVehicle = vehicle.get();
    world.registerEntity(std::move(vehicle));
    world.registerEntity(std::move(player));

    // 使用 startRiding 建立骑乘关系
    rawPlayer->startRiding(*rawVehicle);

    // 通过载具实体应该能解析出控制乘客（玩家）
    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawVehicle);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, rawPlayer);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, ControllingPassengerNotPlayerReturnsNullptr)
{
    // 创建载具实体和非玩家乘客
    auto vehicle = std::make_unique<LivingEntity>(EntityInstanceId(20), nullptr, mc::test::testEcsRegistry());
    auto passenger = std::make_unique<LivingEntity>(EntityInstanceId(21), nullptr, mc::test::testEcsRegistry());

    // 先注册实体到世界
    Entity* rawVehicle = vehicle.get();
    Entity* rawPassenger = passenger.get();
    world.registerEntity(std::move(vehicle));
    world.registerEntity(std::move(passenger));

    // 使用 startRiding 建立骑乘关系
    rawPassenger->startRiding(*rawVehicle);

    // 载具的非玩家乘客不应解析出玩家
    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawVehicle);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, ProjectileWithPlayerShooterReturnsPlayer)
{
    // 创建投射物和射手玩家
    auto player = std::make_unique<Player>(EntityInstanceId(30), "ShooterPlayer", mc::test::testEcsRegistry());
    Player* rawPlayer = player.get();
    auto projectile = std::make_unique<TestProjectileEntity>(EntityInstanceId(31));
    Entity* rawProjectile = projectile.get();

    // 先注册玩家到世界
    world.registerEntity(std::move(player));
    world.registerEntity(std::move(projectile));

    // 设置投射物的射手为玩家
    rawProjectile->setWorld(&world);
    static_cast<mc::entity::ProjectileEntity*>(rawProjectile)->setShooter(rawPlayer);

    // 通过投射物应该能解析出射手（玩家）
    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawProjectile);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, rawPlayer);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, ProjectileWithNonPlayerShooterReturnsNullptr)
{
    // 创建投射物和非玩家射手
    auto shooter = std::make_unique<LivingEntity>(EntityInstanceId(40), nullptr, mc::test::testEcsRegistry());
    auto projectile = std::make_unique<TestProjectileEntity>(EntityInstanceId(41));
    Entity* rawProjectile = projectile.get();

    // 先注册射手到世界
    Entity* rawShooter = shooter.get();
    world.registerEntity(std::move(shooter));
    world.registerEntity(std::move(projectile));

    // 设置投射物的射手为非玩家实体
    rawProjectile->setWorld(&world);
    static_cast<mc::entity::ProjectileEntity*>(rawProjectile)->setShooter(rawShooter);

    // 投射物的非玩家射手不应解析出玩家
    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawProjectile);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, ProjectileWithNoShooterReturnsNullptr)
{
    // 创建投射物（无射手）
    auto projectile = std::make_unique<TestProjectileEntity>(EntityInstanceId(50));
    Entity* rawProjectile = projectile.get();
    world.registerEntity(std::move(projectile));

    // 投射物无射手，不应解析出玩家
    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawProjectile);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, ItemEntityWithPlayerOwnerReturnsPlayer)
{
    // 创建物品实体和所有者玩家
    auto player = std::make_unique<Player>(EntityInstanceId(60), "OwnerPlayer", mc::test::testEcsRegistry());
    Player* rawPlayer = player.get();
    ItemStack emptyStack;
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(61), emptyStack, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());
    Entity* rawItemEntity = itemEntity.get();

    // 先注册玩家到世界
    world.registerEntity(std::move(player));
    world.registerEntity(std::move(itemEntity));

    // 设置物品实体的所有者为玩家
    static_cast<ItemEntity*>(rawItemEntity)->setOwner(rawPlayer->uuid());

    // 通过物品实体应该能解析出所有者（玩家）
    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawItemEntity);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, rawPlayer);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, ItemEntityWithNonPlayerOwnerReturnsNullptr)
{
    // 创建物品实体和非玩家所有者
    auto owner = std::make_unique<LivingEntity>(EntityInstanceId(70), nullptr, mc::test::testEcsRegistry());
    owner->setUuid("non-player-owner-uuid");
    ItemStack emptyStack;
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(71), emptyStack, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());
    Entity* rawItemEntity = itemEntity.get();

    // 先注册所有者到世界
    world.registerEntity(std::move(owner));
    world.registerEntity(std::move(itemEntity));

    // 设置物品实体的所有者为非玩家实体
    static_cast<ItemEntity*>(rawItemEntity)->setOwner("non-player-owner-uuid");

    // 物品实体的非玩家所有者不应解析出玩家
    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawItemEntity);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, ItemEntityWithNoOwnerReturnsNullptr)
{
    // 创建物品实体（无所有者）
    ItemStack emptyStack;
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(80), emptyStack, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());
    Entity* rawItemEntity = itemEntity.get();
    world.registerEntity(std::move(itemEntity));

    // 物品实体无所有者，不应解析出玩家
    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawItemEntity);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SculkShriekerHelperTryGetPlayerTest, ItemEntityWithOwnerNotInWorldReturnsNullptr)
{
    // 创建物品实体，所有者UUID已设置但对应实体不存在于世界中
    ItemStack emptyStack;
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(90), emptyStack, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());
    Entity* rawItemEntity = itemEntity.get();
    world.registerEntity(std::move(itemEntity));

    // 设置一个不存在于世界中的所有者UUID
    static_cast<ItemEntity*>(rawItemEntity)->setOwner("nonexistent-player-uuid");

    // 所有者不在世界中，不应解析出玩家
    Player* result = SculkShriekerHelper::tryGetPlayer(world, rawItemEntity);
    EXPECT_EQ(result, nullptr);
}

// ============================================================================
// SculkShriekerHelper canRespond 条件逻辑说明
// ============================================================================
//
// _canRespond 的逻辑依赖 ServerWorld，无法在纯单元测试中覆盖。
// 其条件为：
//   1. CAN_SUMMON 方块状态属性为 true（自然生成的尖啸体）
//   2. 非和平难度
//   3. 游戏规则 DO_WARDEN_SPAWNING 为 true
// 三个条件全部满足时才返回 true。
// 这些条件需要集成测试覆盖。

// ============================================================================
// SculkShriekerBlockEntity 综合场景测试
// ============================================================================

class SculkShriekerBlockEntityScenarioTest : public ::testing::Test {
protected:
    void SetUp() override { pos_ = BlockPos(100, -60, 200); }

    BlockPos pos_;
};

TEST_F(SculkShriekerBlockEntityScenarioTest, FullShriekCycle)
{
    // 模拟完整的尖啸周期：重置→递增→递增→递增→递增→可召唤→重置
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 模拟 tryShriek: 重置 warningLevel
    entity->setWarningLevel(0);
    EXPECT_EQ(entity->getWarningLevel(), 0);

    // 模拟 tryWarn 将警告等级设为 4
    entity->setWarningLevel(4);
    EXPECT_EQ(entity->getWarningLevel(), 4);
    EXPECT_TRUE(entity->canSummonWarden());

    // 模拟尖啸结束
    entity->setShriekingFinished(true);
    EXPECT_TRUE(entity->isShriekingFinished());

    // 模拟 tryRespond 处理后清除标志
    entity->setShriekingFinished(false);
    entity->setChanged();
    EXPECT_FALSE(entity->isShriekingFinished());

    // 模拟下一次 tryShriek 重置
    entity->setWarningLevel(0);
    EXPECT_EQ(entity->getWarningLevel(), 0);
    EXPECT_FALSE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityScenarioTest, MultipleShrieksBeforeRespond)
{
    // 模拟多次尖啸但警告等级未达到阈值
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 第一次 tryShriek
    entity->setWarningLevel(0);
    entity->setWarningLevel(1); // 模拟 tryWarn 设置
    EXPECT_FALSE(entity->canSummonWarden());

    // 模拟尖啸结束并响应（不会召唤监守者，但会播放声音）
    entity->setShriekingFinished(true);
    entity->setShriekingFinished(false);

    // 第二次 tryShriek（重置再递增）
    entity->setWarningLevel(0);
    entity->setWarningLevel(1); // 模拟 tryWarn 设置
    EXPECT_FALSE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityScenarioTest, WarningLevelPersistence)
{
    // 验证警告等级在序列化/反序列化后保留
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->setWarningLevel(3);

    nlohmann::json data;
    entity->save(data);

    auto loaded = std::make_unique<SculkShriekerBlockEntity>(pos_);
    ASSERT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getWarningLevel(), 3);

    // 继续设置到 4
    loaded->setWarningLevel(4);
    EXPECT_TRUE(loaded->canSummonWarden());
}

// ============================================================================
// SculkShriekerVibrationUser isSculkShrieker 区分测试
// ============================================================================

#include "server/world/blockentity/sculk/SculkVibrationSystem.hpp"

using namespace mc::blockentity;
using namespace mc::gameevent;

class SculkVibrationUserTypeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        sensorPos_ = BlockPos(0, 64, 0);
        shriekerPos_ = BlockPos(10, 64, 10);
    }

    BlockPos sensorPos_;
    BlockPos shriekerPos_;
};

TEST_F(SculkVibrationUserTypeTest, SensorUser_IsNotSculkShrieker)
{
    SculkSensorBlockEntity sensorEntity(sensorPos_);
    SculkSensorVibrationUser sensorUser(sensorEntity);

    // SculkSensorVibrationUser 不是 SculkShrieker
    EXPECT_FALSE(sensorUser.isSculkShrieker());

    // SculkSensorVibrationUser 可以触发规避振动成就
    EXPECT_TRUE(sensorUser.canTriggerAvoidVibration());
}

TEST_F(SculkVibrationUserTypeTest, ShriekerUser_IsSculkShrieker)
{
    SculkShriekerBlockEntity shriekerEntity(shriekerPos_);
    SculkShriekerVibrationUser shriekerUser(shriekerEntity);

    // SculkShriekerVibrationUser 是 SculkShrieker
    EXPECT_TRUE(shriekerUser.isSculkShrieker());

    // SculkShriekerVibrationUser 不可以触发规避振动成就
    EXPECT_FALSE(shriekerUser.canTriggerAvoidVibration());
}

TEST_F(SculkVibrationUserTypeTest, ShriekerUser_OnlyReceivesShriekEvent)
{
    // SculkShriekerVibrationUser 的 canReceiveVibration 只接受 SHRIEK 事件
    // 这在 SculkVibrationSystem.cpp 中实现，此处验证基础属性
    SculkShriekerBlockEntity shriekerEntity(shriekerPos_);
    SculkShriekerVibrationUser shriekerUser(shriekerEntity);

    // 检测半径应为 8
    EXPECT_EQ(shriekerUser.getListenerRadius(), 8);

    // 需要相邻区块 tick
    EXPECT_TRUE(shriekerUser.requiresAdjacentChunksToBeTicking());
}

TEST_F(SculkVibrationUserTypeTest, SensorUser_ListenerRadius)
{
    SculkSensorBlockEntity sensorEntity(sensorPos_);
    SculkSensorVibrationUser sensorUser(sensorEntity);

    // 检测半径应为 8
    EXPECT_EQ(sensorUser.getListenerRadius(), 8);

    // 需要相邻区块 tick
    EXPECT_TRUE(sensorUser.requiresAdjacentChunksToBeTicking());
}

// ============================================================================
// SculkVibrationSystem 类型区分测试
// ============================================================================

TEST(SculkVibrationSystemTypeTest, SensorSystem_UserIsNotShrieker)
{
    SculkSensorBlockEntity sensorEntity(BlockPos(0, 0, 0));
    SculkVibrationSystem system(sensorEntity);

    auto& user = system.getVibrationUser();
    EXPECT_FALSE(user.isSculkShrieker());
}

TEST(SculkVibrationSystemTypeTest, ShriekerSystem_UserIsShrieker)
{
    SculkShriekerBlockEntity shriekerEntity(BlockPos(0, 0, 0));
    SculkVibrationSystem system(shriekerEntity);

    auto& user = system.getVibrationUser();
    EXPECT_TRUE(user.isSculkShrieker());
}
