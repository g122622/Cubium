#include <gtest/gtest.h>

#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/nether/FireBlock.hpp"
#include "world/IWorld.hpp"
#include "world/tick/manager/TickManager.hpp"
#include "world/border/WorldBorder.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/core/EntityType.hpp"
#include "entity/damage/DamageSource.hpp"
#include "core/Constants.hpp"

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 火焰碰撞测试用实体
 *
 * 简单的 LivingEntity 实现，用于测试火焰碰撞伤害
 */
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity(LegacyEntityType type, EntityId id, IWorld* world = nullptr)
        : LivingEntity(type, id, world)
        , m_hurtCount(0)
        , m_lastDamage(0.0f)
        , m_lastDamageType(static_cast<DamageType>(-1))
    {}

    // 重写 hurt 方法以追踪伤害
    bool hurt(DamageSource& source, f32 amount) override {
        m_hurtCount++;
        m_lastDamage = amount;
        m_lastDamageType = source.type();
        return LivingEntity::hurt(source, amount);
    }

    [[nodiscard]] i32 hurtCount() const { return m_hurtCount; }
    [[nodiscard]] f32 lastDamage() const { return m_lastDamage; }
    [[nodiscard]] DamageType lastDamageType() const { return m_lastDamageType; }

    void resetHurtStats() {
        m_hurtCount = 0;
        m_lastDamage = 0.0f;
        m_lastDamageType = static_cast<DamageType>(-1);
    }

protected:
    i32 m_hurtCount;
    f32 m_lastDamage;
    DamageType m_lastDamageType;
};

/**
 * @brief 火焰免疫测试用实体
 */
class FireImmuneTestEntity : public TestLivingEntity {
public:
    FireImmuneTestEntity(LegacyEntityType type, EntityId id, IWorld* world = nullptr)
        : TestLivingEntity(type, id, world)
        , m_immuneToFire(true)
    {}

    [[nodiscard]] bool isImmuneToFire() const override {
        return m_immuneToFire;
    }

    void setImmuneToFire(bool immune) {
        m_immuneToFire = immune;
    }

private:
    bool m_immuneToFire;
};

/**
 * @brief 火焰测试用世界
 */
class FireTestWorld final : public IWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }
    [[nodiscard]] bool isUltraWarm() const override { return false; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) {
        (void)setBlockState(pos.x, pos.y, pos.z, state);
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        const_cast<FireTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    void ensureTickManager() const {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(const_cast<FireTestWorld&>(*this));
        }
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    mutable std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

class FireBlockCollisionTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

// ========== Entity 火焰方法测试 ==========

TEST_F(FireBlockCollisionTest, Entity_GetFireTimer_EqualsFire) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);

    // 初始值应为 0
    EXPECT_EQ(entity.fire(), 0);
    EXPECT_EQ(entity.getFireTimer(), 0);

    // 设置火焰（setFire 接收 ticks）
    entity.setFire(100);  // 100 ticks = 5 秒
    EXPECT_EQ(entity.fire(), 100);
    EXPECT_EQ(entity.getFireTimer(), 100);

    // getFireTimer 应该与 fire 相同
    entity.forceFireTicks(50);
    EXPECT_EQ(entity.fire(), 50);
    EXPECT_EQ(entity.getFireTimer(), 50);
}

TEST_F(FireBlockCollisionTest, Entity_SetFire_OnlyIncreases) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);

    // 设置初始火焰
    entity.setFire(100);  // 100 ticks
    EXPECT_EQ(entity.fire(), 100);

    // 尝试设置更小的值，不应改变
    entity.setFire(60);   // 60 ticks < 100 ticks
    EXPECT_EQ(entity.fire(), 100);  // 保持 100

    // 设置更大的值，应该更新
    entity.setFire(200);  // 200 ticks > 100 ticks
    EXPECT_EQ(entity.fire(), 200);  // 更新为 200
}

TEST_F(FireBlockCollisionTest, Entity_ForceFireTicks_SetsDirectly) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);

    entity.setFire(200);
    EXPECT_EQ(entity.fire(), 200);

    // forceFireTicks 可以减少值
    entity.forceFireTicks(50);
    EXPECT_EQ(entity.fire(), 50);

    // forceFireTicks 可以设置为负值（用于短暂火焰免疫期）
    entity.forceFireTicks(-10);
    EXPECT_EQ(entity.fire(), -10);

    // forceFireTicks 可以设置为 0
    entity.forceFireTicks(0);
    EXPECT_EQ(entity.fire(), 0);
}

TEST_F(FireBlockCollisionTest, Entity_IsImmuneToFire_DefaultFalse) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);

    // 默认情况下，实体不免疫火焰（取决于 EntityType）
    // TestLivingEntity 没有注册到 EntityRegistry，所以默认返回 false
    EXPECT_FALSE(entity.isImmuneToFire());
}

TEST_F(FireBlockCollisionTest, Entity_IsImmuneToFire_Overrideable) {
    FireTestWorld world;
    FireImmuneTestEntity immuneEntity(LegacyEntityType::Blaze, EntityId(1), &world);

    // 默认免疫
    EXPECT_TRUE(immuneEntity.isImmuneToFire());

    // 可以关闭免疫
    immuneEntity.setImmuneToFire(false);
    EXPECT_FALSE(immuneEntity.isImmuneToFire());

    // 可以重新开启
    immuneEntity.setImmuneToFire(true);
    EXPECT_TRUE(immuneEntity.isImmuneToFire());
}

// ========== FireBlock::onEntityCollision 测试 ==========

TEST_F(FireBlockCollisionTest, OnEntityCollision_ImmuneEntity_NoDamage) {
    FireTestWorld world;
    FireImmuneTestEntity entity(LegacyEntityType::Blaze, EntityId(1), &world);
    entity.setImmuneToFire(true);

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 免疫实体不受伤害
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);

    EXPECT_EQ(entity.hurtCount(), 0);
    EXPECT_EQ(entity.fire(), 0);  // 火焰计时器不变
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_NormalEntity_TakesDamage) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);
    entity.setHealth(20.0f);  // 设置生命值

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 普通实体受到伤害
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);

    EXPECT_EQ(entity.hurtCount(), 1);
    EXPECT_EQ(entity.lastDamage(), 1.0f);  // 普通火焰伤害 1.0
    EXPECT_EQ(entity.lastDamageType(), DamageType::InFire);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_IncrementsFireTimer) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 初始火焰计时器为 0
    EXPECT_EQ(entity.getFireTimer(), 0);

    // 第一次碰撞
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);
    EXPECT_EQ(entity.getFireTimer(), 1);

    // 第二次碰撞
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);
    EXPECT_EQ(entity.getFireTimer(), 2);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_ImmunityEndIgnitesEntity) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 设置火焰计时器为 -1（模拟短暂免疫期即将结束）
    // MC 1.16.5: 当 timer 为负时，表示实体处于短暂火焰免疫期
    entity.forceFireTicks(-1);

    // 碰撞时 timer 从 -1 增加到 0，然后触发 setFire(160)
    // MC 1.16.5 逻辑：
    // 1. forceFireTicks(getFireTimer() + 1) → timer 从 -1 变为 0
    // 2. if (getFireTimer() == 0) → true
    // 3. setFire(160) → timer 被设置为 160
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);

    // timer 最终变为 160（8 秒 = 160 ticks）
    EXPECT_EQ(entity.getFireTimer(), 160);
    EXPECT_EQ(entity.fire(), 160);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_NegativeTimerIgnites) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 设置火焰计时器为 -5（深度免疫期）
    entity.forceFireTicks(-5);

    // 连续碰撞直到 timer 变为 0 并触发点燃
    // -5 → -4 → -3 → -2 → -1 → 0（触发 setFire）
    for (int i = 0; i < 5; ++i) {
        VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);
    }

    // 第 5 次碰撞时 timer 从 -1 变为 0，触发 setFire(160)
    // 所以 timer 最终变为 160
    EXPECT_EQ(entity.getFireTimer(), 160);
    EXPECT_EQ(entity.fire(), 160);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_FirstCollisionNotIgnite) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 初始 timer 为 0
    EXPECT_EQ(entity.getFireTimer(), 0);

    // 第一次碰撞：timer 从 0 变为 1，不触发 setFire
    // 因为 MC 1.16.5 逻辑是：先 increment，再检查是否等于 0
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);

    EXPECT_EQ(entity.getFireTimer(), 1);
    // fire() 不会被设置为 160，因为 timer != 0
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_SoulFire_HigherDamage) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);
    entity.setHealth(20.0f);

    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);
    const BlockState& soulFireState = VanillaBlocks::SOUL_FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 灵魂火造成 2.0 伤害
    // SoulFireBlock 继承自 FireBlock，m_fireDamage = 2
    FireBlock* soulFireBlock = const_cast<FireBlock*>(static_cast<const FireBlock*>(VanillaBlocks::SOUL_FIRE));
    soulFireBlock->onEntityCollision(soulFireState, world, pos, entity);

    EXPECT_EQ(entity.hurtCount(), 1);
    EXPECT_EQ(entity.lastDamage(), 2.0f);  // 灵魂火伤害 2.0
    EXPECT_EQ(entity.lastDamageType(), DamageType::InFire);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_NonLivingEntity_TimerIncreases) {
    FireTestWorld world;

    // Entity 基类不是 LivingEntity，不会受到伤害
    Entity entity(LegacyEntityType::Item, EntityId(1), &world);

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // Entity 基类不受伤害（dynamic_cast<LivingEntity*> 返回 nullptr）
    // 但火焰计时器仍然增加
    EXPECT_EQ(entity.getFireTimer(), 0);
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);
    EXPECT_EQ(entity.getFireTimer(), 1);  // 计时器增加
    // isOnFire() 检查 fire > 0，fireTimer 变为 1 所以点燃了
    EXPECT_TRUE(entity.isOnFire());
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_MultipleCollisions_EachTick) {
    FireTestWorld world;
    TestLivingEntity entity(LegacyEntityType::Pig, EntityId(1), &world);
    entity.setHealth(100.0f);  // 高生命值以承受多次伤害

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 模拟实体在火焰中停留多个 tick
    for (int i = 0; i < 5; ++i) {
        VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);
    }

    // 应该受到 5 次伤害
    EXPECT_EQ(entity.hurtCount(), 5);
    EXPECT_EQ(entity.lastDamage(), 1.0f);

    // 火焰计时器应该增加 5
    EXPECT_EQ(entity.getFireTimer(), 5);
}

} // namespace
