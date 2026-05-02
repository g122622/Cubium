#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/items/weapon/BowItem.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/item/items/weapon/ArrowItem.hpp"
#include "common/item/items/weapon/TridentItem.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用世界存根
 */
class WeaponTestWorld final : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlock(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return fluid::Fluid::getFluidState(0);
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("WeaponTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("WeaponTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override {
        m_spawnedEntities.push_back(entity.get());
        m_ownedEntities.push_back(std::move(entity));
        return ++m_lastEntityId;
    }

    void addParticle(client::renderer::trident::particle::ParticleTypeId,
                     const Vector3&,
                     const Vector3&,
                     const Vector3& = Vector3(0, 0, 0),
                     u32 = 1) override {
        // 测试中忽略粒子效果
    }

    [[nodiscard]] const std::vector<Entity*>& spawnedEntities() const { return m_spawnedEntities; }

    void clearSpawnedEntities() {
        m_spawnedEntities.clear();
        m_ownedEntities.clear();
    }

private:
    math::Random m_random{12345};
    EntityId m_lastEntityId = 0;
    std::vector<Entity*> m_spawnedEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
};

class WeaponItemTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
    }

    void TearDown() override {
        // Items 清理由静态析构处理
    }

    WeaponTestWorld m_world;
};

// ============================================================================
// BowItem 测试
// ============================================================================

TEST_F(WeaponItemTest, BowItem_Registered_HasCorrectProperties) {
    ASSERT_NE(Items::BOW, nullptr);
    EXPECT_EQ(Items::BOW->maxStackSize(), 1);
    EXPECT_GT(Items::BOW->maxDamage(), 0);  // 弓有耐久度
}

TEST_F(WeaponItemTest, BowItem_GetUseDuration) {
    auto* bow = dynamic_cast<const item::BowItem*>(Items::BOW);
    ASSERT_NE(bow, nullptr);

    ItemStack stack(Items::BOW, 1);
    EXPECT_EQ(bow->getUseDuration(stack), 72000);  // MC 1.16.5: 几乎无限制
}

TEST_F(WeaponItemTest, BowItem_GetUseAction) {
    auto* bow = dynamic_cast<const item::BowItem*>(Items::BOW);
    ASSERT_NE(bow, nullptr);

    ItemStack stack(Items::BOW, 1);
    EXPECT_EQ(bow->getUseAction(stack), UseAction::Bow);
}

TEST_F(WeaponItemTest, BowItem_GetArrowVelocity) {
    // MC 1.16.5 公式: f = charge / 20.0, velocity = (f * f + f * 2.0) / 3.0

    // 0 tick: 速度 0
    EXPECT_FLOAT_EQ(item::BowItem::getArrowVelocity(0), 0.0f);

    // 1 tick: 速度 ~0.005
    f32 v1 = item::BowItem::getArrowVelocity(1);
    EXPECT_GT(v1, 0.0f);
    EXPECT_LT(v1, 0.1f);

    // 10 tick: 速度计算
    f32 v10 = item::BowItem::getArrowVelocity(10);
    EXPECT_GT(v10, v1);

    // 20 tick (满蓄力): 速度 1.0
    EXPECT_FLOAT_EQ(item::BowItem::getArrowVelocity(20), 1.0f);

    // 超过 20 tick: 最大 1.0
    EXPECT_FLOAT_EQ(item::BowItem::getArrowVelocity(25), 1.0f);
    EXPECT_FLOAT_EQ(item::BowItem::getArrowVelocity(100), 1.0f);
}

TEST_F(WeaponItemTest, BowItem_GetAmmoPredicate) {
    auto* bow = dynamic_cast<const item::BowItem*>(Items::BOW);
    ASSERT_NE(bow, nullptr);

    auto predicate = bow->getAmmoPredicate();

    // 箭矢应该被接受
    ItemStack arrow(Items::ARROW, 1);
    EXPECT_TRUE(predicate(arrow));

    // 空物品应该被拒绝
    EXPECT_FALSE(predicate(ItemStack::EMPTY));

    // 非箭矢物品应该被拒绝
    ItemStack stone(Items::STONE, 1);
    EXPECT_FALSE(predicate(stone));
}

// ============================================================================
// CrossbowItem 测试
// ============================================================================

TEST_F(WeaponItemTest, CrossbowItem_Registered_HasCorrectProperties) {
    ASSERT_NE(Items::CROSSBOW, nullptr);
    EXPECT_EQ(Items::CROSSBOW->maxStackSize(), 1);
    EXPECT_GT(Items::CROSSBOW->maxDamage(), 0);  // 弩有耐久度
}

TEST_F(WeaponItemTest, CrossbowItem_GetUseDuration) {
    auto* crossbow = dynamic_cast<const item::CrossbowItem*>(Items::CROSSBOW);
    ASSERT_NE(crossbow, nullptr);

    ItemStack stack(Items::CROSSBOW, 1);
    // 基础装填时间 25 tick + 3 = 28 tick
    EXPECT_EQ(crossbow->getUseDuration(stack), 28);
}

TEST_F(WeaponItemTest, CrossbowItem_GetUseAction) {
    auto* crossbow = dynamic_cast<const item::CrossbowItem*>(Items::CROSSBOW);
    ASSERT_NE(crossbow, nullptr);

    ItemStack stack(Items::CROSSBOW, 1);
    EXPECT_EQ(crossbow->getUseAction(stack), UseAction::Crossbow);
}

TEST_F(WeaponItemTest, CrossbowItem_GetChargeTime) {
    auto* crossbow = dynamic_cast<const item::CrossbowItem*>(Items::CROSSBOW);
    ASSERT_NE(crossbow, nullptr);

    ItemStack stack(Items::CROSSBOW, 1);

    // 基础装填时间 25 tick
    EXPECT_EQ(item::CrossbowItem::getChargeTime(stack), 25);

    // TODO: 测试快速装填附魔减少装填时间
}

TEST_F(WeaponItemTest, CrossbowItem_IsCharged) {
    auto* crossbow = dynamic_cast<const item::CrossbowItem*>(Items::CROSSBOW);
    ASSERT_NE(crossbow, nullptr);

    ItemStack stack(Items::CROSSBOW, 1);

    // 初始状态：未装填
    EXPECT_FALSE(item::CrossbowItem::isCharged(stack));

    // 设置为装填状态
    item::CrossbowItem::setCharged(stack, true);
    EXPECT_TRUE(item::CrossbowItem::isCharged(stack));

    // 设置为未装填状态
    item::CrossbowItem::setCharged(stack, false);
    EXPECT_FALSE(item::CrossbowItem::isCharged(stack));
}

TEST_F(WeaponItemTest, CrossbowItem_GetAmmoPredicate) {
    auto* crossbow = dynamic_cast<const item::CrossbowItem*>(Items::CROSSBOW);
    ASSERT_NE(crossbow, nullptr);

    auto predicate = crossbow->getAmmoPredicate();

    // 箭矢应该被接受
    ItemStack arrow(Items::ARROW, 1);
    EXPECT_TRUE(predicate(arrow));

    // 烟花火箭应该被接受
    if (Items::FIREWORK_ROCKET != nullptr) {
        ItemStack firework(Items::FIREWORK_ROCKET, 1);
        EXPECT_TRUE(predicate(firework));
    }

    // 空物品应该被拒绝
    EXPECT_FALSE(predicate(ItemStack::EMPTY));

    // 非弹药物品应该被拒绝
    ItemStack stone(Items::STONE, 1);
    EXPECT_FALSE(predicate(stone));
}

TEST_F(WeaponItemTest, CrossbowItem_GetInventoryAmmoPredicate) {
    auto* crossbow = dynamic_cast<const item::CrossbowItem*>(Items::CROSSBOW);
    ASSERT_NE(crossbow, nullptr);

    auto predicate = crossbow->getInventoryAmmoPredicate();

    // 箭矢应该被接受
    ItemStack arrow(Items::ARROW, 1);
    EXPECT_TRUE(predicate(arrow));

    // 空物品应该被拒绝
    EXPECT_FALSE(predicate(ItemStack::EMPTY));

    // 烟花火箭不应该被背包弹药预测接受（只能放在副手）
    if (Items::FIREWORK_ROCKET != nullptr) {
        ItemStack firework(Items::FIREWORK_ROCKET, 1);
        EXPECT_FALSE(predicate(firework));
    }
}

// ============================================================================
// ArrowItem 测试
// ============================================================================

TEST_F(WeaponItemTest, ArrowItem_Registered_HasCorrectProperties) {
    ASSERT_NE(Items::ARROW, nullptr);
    EXPECT_EQ(Items::ARROW->maxStackSize(), 64);  // 箭矢可堆叠到 64
}

TEST_F(WeaponItemTest, ArrowItem_IsInfinite) {
    auto* arrow = dynamic_cast<const item::ArrowItem*>(Items::ARROW);
    ASSERT_NE(arrow, nullptr);

    ItemStack arrowStack(Items::ARROW, 1);
    ItemStack bowStack(Items::BOW, 1);

    // 无附魔弓：箭不是无限的
    // 注意：需要 Player 对象来测试，这里只测试基本逻辑
}

// ============================================================================
// 耐久度测试
// ============================================================================

TEST_F(WeaponItemTest, BowItem_HasDurability) {
    ASSERT_NE(Items::BOW, nullptr);
    EXPECT_EQ(Items::BOW->maxDamage(), 384);  // MC 1.16.5: 弓有 384 点耐久
}

TEST_F(WeaponItemTest, CrossbowItem_HasDurability) {
    ASSERT_NE(Items::CROSSBOW, nullptr);
    EXPECT_EQ(Items::CROSSBOW->maxDamage(), 326);  // MC 1.16.5: 弩有 326 点耐久
}

TEST_F(WeaponItemTest, TridentItem_HasDurability) {
    ASSERT_NE(Items::TRIDENT, nullptr);
    EXPECT_EQ(Items::TRIDENT->maxDamage(), 250);  // MC 1.16.5: 三叉戟有 250 点耐久
}

// ============================================================================
// 使用动作测试
// ============================================================================

TEST_F(WeaponItemTest, TridentItem_GetUseAction) {
    auto* trident = dynamic_cast<const item::TridentItem*>(Items::TRIDENT);
    ASSERT_NE(trident, nullptr);

    ItemStack stack(Items::TRIDENT, 1);
    EXPECT_EQ(trident->getUseAction(stack), UseAction::Spear);
}

TEST_F(WeaponItemTest, TridentItem_GetUseDuration) {
    auto* trident = dynamic_cast<const item::TridentItem*>(Items::TRIDENT);
    ASSERT_NE(trident, nullptr);

    ItemStack stack(Items::TRIDENT, 1);
    EXPECT_EQ(trident->getUseDuration(stack), 72000);  // MC 1.16.5: 几乎无限制
}

} // namespace
} // namespace mc
