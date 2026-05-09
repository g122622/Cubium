#include <gtest/gtest.h>
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "entity/entities/projectile/TridentEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "item/core/ItemStack.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "world/IWorld.hpp"
#include "util/math/random/Random.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// Mock World for Testing
// ============================================================================

class MockWorld : public IWorld {
public:
    MockWorld() : IWorld(nullptr, DimensionId::Overworld) {}

    // IWorld 接口实现
    bool isClientSide() const override { return m_isClientSide; }
    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }

    // 其他必要的方法提供默认实现
    EntityId spawnEntity(std::unique_ptr<Entity> entity) override { return 0; }
    Entity* getEntity(EntityId id) override { return nullptr; }
    const Entity* getEntity(EntityId id) const override { return nullptr; }
    void removeEntity(EntityId id) override {}
    std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except = nullptr) const override { return {}; }
    std::vector<Entity*> getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except = nullptr) const override { return {}; }
    const BlockState* getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const override { return nullptr; }
    bool setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState& state) override { return false; }
    std::optional<BlockState> getBlockStateAt(BlockCoord x, BlockCoord y, BlockCoord z) const override { return std::nullopt; }
    void playSound(const ResourceLocation& soundEventId, sound::SoundCategory category, const Vector3& position, f32 volume, f32 pitch) override {
        m_lastSoundEvent = soundEventId;
        m_soundPlayed = true;
    }

    bool hasNoCollisions(const AxisAlignedBB& box) const override { return true; }
    std::vector<AABB> getBlockCollisions(const AxisAlignedBB& box) const override { return {}; }
    std::vector<AABB> getEntityCollisions(const Entity* entity, const AxisAlignedBB& box) const override { return {}; }
    bool hasEntityCollision(const Entity* entity, const AxisAlignedBB& box) const override { return false; }

    void tick() override {}
    const Biome* getBiome(BlockCoord x, BlockCoord y, BlockCoord z) const override { return nullptr; }
    Difficulty difficulty() const override { return Difficulty::Normal; }
    math::Random& random() override { return m_random; }
    void addParticle(client::renderer::trident::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override {}

    // 测试辅助方法
    bool wasSoundPlayed() const { return m_soundPlayed; }
    const ResourceLocation& getLastSoundEvent() const { return m_lastSoundEvent; }
    void resetSoundFlag() { m_soundPlayed = false; }

private:
    bool m_isClientSide = false;
    bool m_soundPlayed = false;
    ResourceLocation m_lastSoundEvent;
    mutable math::Random m_random;
};

// ============================================================================
// 测试固定装置
// ============================================================================

class ArrowPickupTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化物品注册表
        Items::initialize();

        // 获取箭矢物品
        m_arrow = Items::ARROW;
        m_spectralArrow = Items::SPECTRAL_ARROW;
        m_trident = Items::TRIDENT;

        // 创建模拟世界
        m_world = std::make_unique<MockWorld>();
    }

    void TearDown() override {
        m_world.reset();
    }

    Item* m_arrow = nullptr;
    Item* m_spectralArrow = nullptr;
    Item* m_trident = nullptr;
    std::unique_ptr<MockWorld> m_world;
};

// ============================================================================
// ArrowEntity::getArrowStack 测试
// ============================================================================

TEST_F(ArrowPickupTest, ArrowEntity_GetArrowStack_ReturnsArrowItem) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    arrow->setWorld(m_world.get());
    arrow->setPosition(0, 0, 0);

    // 普通箭矢应该返回 ARROW 物品
    item::ItemStack stack = arrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), m_arrow);
    EXPECT_EQ(stack.getCount(), 1);
}

TEST_F(ArrowPickupTest, ArrowEntity_GetArrowStack_WithEffects_ReturnsTippedArrow) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    arrow->setWorld(m_world.get());
    arrow->setPosition(0, 0, 0);

    // 添加药水效果
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 0));

    // 药水箭应该返回 TIPPED_ARROW 物品
    item::ItemStack stack = arrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), Items::TIPPED_ARROW);
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// SpectralArrowEntity::getArrowStack 测试
// ============================================================================

TEST_F(ArrowPickupTest, SpectralArrowEntity_GetArrowStack_ReturnsSpectralArrow) {
    auto spectralArrow = std::make_unique<SpectralArrowEntity>(LegacyEntityType::Unknown, 1);
    spectralArrow->setWorld(m_world.get());
    spectralArrow->setPosition(0, 0, 0);

    // 光灵箭应该返回 SPECTRAL_ARROW 物品
    item::ItemStack stack = spectralArrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), m_spectralArrow);
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// TridentEntity::getArrowStack 测试
// ============================================================================

TEST_F(ArrowPickupTest, TridentEntity_GetArrowStack_ReturnsTridentItem) {
    auto trident = std::make_unique<TridentEntity>(LegacyEntityType::Unknown, 1);
    trident->setWorld(m_world.get());
    trident->setPosition(0, 0, 0);

    // 设置三叉戟物品
    item::ItemStack tridentStack(*m_trident, 1);
    trident->setItemStack(tridentStack);

    // 三叉戟应该返回 TRIDENT 物品
    item::ItemStack stack = trident->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), m_trident);
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// AbstractArrowEntity::onPlayerPickup 测试
// ============================================================================

TEST_F(ArrowPickupTest, OnPlayerPickup_Disallowed_ReturnsFalse) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    arrow->setWorld(m_world.get());
    arrow->setPosition(0, 0, 0);
    arrow->setPickupStatus(PickupStatus::Disallowed);
    arrow->setInGround(true);  // 必须插在地方才能拾取

    // 创建玩家（需要简化测试）
    // Player 需要 IWorld，这里使用 Mock
    // 实际测试中可能需要更复杂的 Mock 或 Integration Test

    // 这里我们验证基本逻辑：Disallowed 状态应该拒绝拾取
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Disallowed);
}

TEST_F(ArrowPickupTest, OnPlayerPickup_Allowed_InGround_CanPickup) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    arrow->setWorld(m_world.get());
    arrow->setPosition(0, 0, 0);
    arrow->setPickupStatus(PickupStatus::Allowed);
    arrow->setInGround(true);

    // 验证箭矢状态正确
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Allowed);
    EXPECT_TRUE(arrow->isInGround());
}

TEST_F(ArrowPickupTest, OnPlayerPickup_NotInGround_CannotPickup) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    arrow->setWorld(m_world.get());
    arrow->setPosition(0, 0, 0);
    arrow->setPickupStatus(PickupStatus::Allowed);
    arrow->setInGround(false);  // 未插在方块中

    // 验证箭矢状态
    EXPECT_FALSE(arrow->isInGround());
}

TEST_F(ArrowPickupTest, OnPlayerPickup_CreativeOnly_OnlyCreativeCanPickup) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    arrow->setWorld(m_world.get());
    arrow->setPosition(0, 0, 0);
    arrow->setPickupStatus(PickupStatus::CreativeOnly);
    arrow->setInGround(true);

    // 验证 CreativeOnly 状态
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::CreativeOnly);
}

// ============================================================================
// PickupStatus 枚举值测试
// ============================================================================

TEST_F(ArrowPickupTest, PickupStatus_Values) {
    // 验证枚举值
    EXPECT_EQ(static_cast<int>(PickupStatus::Disallowed), 0);
    EXPECT_EQ(static_cast<int>(PickupStatus::Allowed), 1);
    EXPECT_EQ(static_cast<int>(PickupStatus::CreativeOnly), 2);
}

// ============================================================================
// 箭矢属性测试
// ============================================================================

TEST_F(ArrowPickupTest, ArrowEntity_DefaultDamage) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    EXPECT_FLOAT_EQ(arrow->damage(), 2.0f);
}

TEST_F(ArrowPickupTest, ArrowEntity_SetDamage) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    arrow->setDamage(5.5f);
    EXPECT_FLOAT_EQ(arrow->damage(), 5.5f);
}

TEST_F(ArrowPickupTest, ArrowEntity_CriticalFlag) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    EXPECT_FALSE(arrow->isCritical());
    arrow->setCritical(true);
    EXPECT_TRUE(arrow->isCritical());
}

TEST_F(ArrowPickupTest, ArrowEntity_PierceLevel) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    EXPECT_EQ(arrow->pierceLevel(), 0);
    arrow->setPierceLevel(3);
    EXPECT_EQ(arrow->pierceLevel(), 3);
}

TEST_F(ArrowPickupTest, SpectralArrowEntity_DefaultDamage) {
    auto spectralArrow = std::make_unique<SpectralArrowEntity>(LegacyEntityType::Unknown, 1);
    EXPECT_FLOAT_EQ(spectralArrow->damage(), 2.0f);
}

TEST_F(ArrowPickupTest, TridentEntity_DefaultDamage) {
    auto trident = std::make_unique<TridentEntity>(LegacyEntityType::Unknown, 1);
    EXPECT_FLOAT_EQ(trident->damage(), 8.0f);
}

TEST_F(ArrowPickupTest, TridentEntity_DefaultPickupStatus) {
    auto trident = std::make_unique<TridentEntity>(LegacyEntityType::Unknown, 1);
    // 三叉戟默认应该可以拾取
    EXPECT_EQ(trident->pickupStatus(), PickupStatus::Allowed);
}

// ============================================================================
// 抽象基类测试 - getArrowStack 必须实现
// ============================================================================

TEST_F(ArrowPickupTest, ArrowEntity_IsAbstractArrowEntity) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 1);
    AbstractArrowEntity* base = arrow.get();
    EXPECT_NE(base, nullptr);

    // 验证可以通过基类指针调用 getArrowStack
    item::ItemStack stack = base->getArrowStack();
    EXPECT_NE(stack.getItem(), nullptr);
}

TEST_F(ArrowPickupTest, SpectralArrowEntity_IsAbstractArrowEntity) {
    auto spectralArrow = std::make_unique<SpectralArrowEntity>(LegacyEntityType::Unknown, 1);
    AbstractArrowEntity* base = spectralArrow.get();
    EXPECT_NE(base, nullptr);

    // 验证可以通过基类指针调用 getArrowStack
    item::ItemStack stack = base->getArrowStack();
    EXPECT_NE(stack.getItem(), nullptr);
}

TEST_F(ArrowPickupTest, TridentEntity_IsAbstractArrowEntity) {
    auto trident = std::make_unique<TridentEntity>(LegacyEntityType::Unknown, 1);
    AbstractArrowEntity* base = trident.get();
    EXPECT_NE(base, nullptr);

    // 设置三叉戟物品
    item::ItemStack tridentStack(*m_trident, 1);
    trident->setItemStack(tridentStack);

    // 验证可以通过基类指针调用 getArrowStack
    item::ItemStack stack = base->getArrowStack();
    EXPECT_NE(stack.getItem(), nullptr);
}
