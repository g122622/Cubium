#include <gtest/gtest.h>

#include "common/entity/entities/passive/special/FoxEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"
#include "common/TestWorldHelper.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用模拟世界
 */
class FoxTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("FoxTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("FoxTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class FoxEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    FoxTestWorld m_world;
};

// ========== 狐狸类型测试 ==========

TEST_F(FoxEntityTest, FoxType_DefaultIsRed) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);
    // 默认类型是红色狐狸
    EXPECT_EQ(fox.getFoxType(), FoxEntity::FoxType::Red);
}

TEST_F(FoxEntityTest, FoxType_CanSetAndGetType) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    fox.setFoxType(FoxEntity::FoxType::Snow);
    EXPECT_EQ(fox.getFoxType(), FoxEntity::FoxType::Snow);

    fox.setFoxType(FoxEntity::FoxType::Red);
    EXPECT_EQ(fox.getFoxType(), FoxEntity::FoxType::Red);
}

// ========== 信任系统测试 ==========

TEST_F(FoxEntityTest, TrustSystem_NoTrustedPlayersInitially) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    EXPECT_FALSE(fox.trusts(12345));
    EXPECT_FALSE(fox.getFirstTrustedPlayer().has_value());
}

TEST_F(FoxEntityTest, TrustSystem_CanAddTrustedPlayer) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    fox.addTrustedPlayer(12345);
    EXPECT_TRUE(fox.trusts(12345));
    EXPECT_EQ(fox.getFirstTrustedPlayer().value_or(0), 12345);
}

TEST_F(FoxEntityTest, TrustSystem_CanAddMultipleTrustedPlayers) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    fox.addTrustedPlayer(111);
    fox.addTrustedPlayer(222);

    EXPECT_TRUE(fox.trusts(111));
    EXPECT_TRUE(fox.trusts(222));
    EXPECT_EQ(fox.getFirstTrustedPlayer().value_or(0), 111);
}

TEST_F(FoxEntityTest, TrustSystem_MaxTwoTrustedPlayers) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    fox.addTrustedPlayer(111);
    fox.addTrustedPlayer(222);
    fox.addTrustedPlayer(333);  // 应该替换第一个

    EXPECT_FALSE(fox.trusts(111));  // 第一个被替换
    EXPECT_TRUE(fox.trusts(222));
    EXPECT_TRUE(fox.trusts(333));
}

TEST_F(FoxEntityTest, TrustSystem_CanRemoveTrustedPlayer) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    fox.addTrustedPlayer(12345);
    EXPECT_TRUE(fox.trusts(12345));

    fox.removeTrustedPlayer(12345);
    EXPECT_FALSE(fox.trusts(12345));
}

TEST_F(FoxEntityTest, TrustSystem_DoesNotAddDuplicate) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    fox.addTrustedPlayer(12345);
    fox.addTrustedPlayer(12345);  // 重复添加

    // 应该只有一个
    EXPECT_TRUE(fox.trusts(12345));
    fox.removeTrustedPlayer(12345);
    EXPECT_FALSE(fox.trusts(12345));
}

// ========== 繁殖物品测试 ==========

TEST_F(FoxEntityTest, IsBreedingItem_AcceptsSweetBerries) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    ItemStack sweetBerriesStack(Items::SWEET_BERRIES, 1);
    EXPECT_TRUE(fox.isBreedingItem(sweetBerriesStack));
}

TEST_F(FoxEntityTest, IsBreedingItem_RejectsOtherItems) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    // 测试不接受其他物品
    if (Items::WHEAT != nullptr) {
        ItemStack wheatStack(Items::WHEAT, 1);
        EXPECT_FALSE(fox.isBreedingItem(wheatStack));
    }

    if (Items::CARROT != nullptr) {
        ItemStack carrotStack(Items::CARROT, 1);
        EXPECT_FALSE(fox.isBreedingItem(carrotStack));
    }

    // 空物品栈
    ItemStack emptyStack;
    EXPECT_FALSE(fox.isBreedingItem(emptyStack));
}

// ========== spawnBaby 测试 ==========

TEST_F(FoxEntityTest, SpawnBaby_CreatesChildFox) {
    FoxEntity parent1(LegacyEntityType::Unknown, 1);
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setFoxType(FoxEntity::FoxType::Red);

    FoxEntity parent2(LegacyEntityType::Unknown, 2);
    parent2.setFoxType(FoxEntity::FoxType::Snow);

    auto baby = parent1.spawnBaby(parent2);

    ASSERT_NE(baby, nullptr);
    EXPECT_TRUE(baby->isChild());

    // 检查是 FoxEntity 类型
    FoxEntity* babyFox = dynamic_cast<FoxEntity*>(baby.get());
    EXPECT_NE(babyFox, nullptr);
}

TEST_F(FoxEntityTest, SpawnBaby_InheritsParentType) {
    FoxEntity parent1(LegacyEntityType::Unknown, 1);
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setFoxType(FoxEntity::FoxType::Red);

    FoxEntity parent2(LegacyEntityType::Unknown, 2);
    parent2.setFoxType(FoxEntity::FoxType::Snow);

    // 多次测试类型继承（由于随机性，50%概率继承任一父母）
    int redCount = 0;
    int snowCount = 0;
    const int iterations = 100;

    for (int i = 0; i < iterations; ++i) {
        // 每次创建新的父实体来获得不同的随机种子
        FoxEntity p1(LegacyEntityType::Unknown, static_cast<EntityId>(i * 2 + 1));
        p1.setWorld(&m_world);
        p1.setPosition(0.0f, 64.0f, 0.0f);
        p1.setFoxType(FoxEntity::FoxType::Red);

        FoxEntity p2(LegacyEntityType::Unknown, static_cast<EntityId>(i * 2 + 2));
        p2.setFoxType(FoxEntity::FoxType::Snow);

        auto baby = p1.spawnBaby(p2);
        ASSERT_NE(baby, nullptr);

        FoxEntity* babyFox = dynamic_cast<FoxEntity*>(baby.get());
        ASSERT_NE(babyFox, nullptr);

        if (babyFox->getFoxType() == FoxEntity::FoxType::Red) {
            redCount++;
        } else if (babyFox->getFoxType() == FoxEntity::FoxType::Snow) {
            snowCount++;
        }
    }

    // 两种类型都应该出现（概率分布测试）
    // 由于是50%概率，两种类型都应该有相当的数量
    EXPECT_GT(redCount, 10) << "Red type should appear at least 10 times in 100 iterations";
    EXPECT_GT(snowCount, 10) << "Snow type should appear at least 10 times in 100 iterations";
}

TEST_F(FoxEntityTest, SpawnBaby_InheritsTrustedPlayers) {
    FoxEntity parent1(LegacyEntityType::Unknown, 1);
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setFoxType(FoxEntity::FoxType::Red);
    parent1.addTrustedPlayer(111);
    parent1.addTrustedPlayer(222);

    FoxEntity parent2(LegacyEntityType::Unknown, 2);
    parent2.setWorld(&m_world);  // 设置世界以获得随机数
    parent2.setFoxType(FoxEntity::FoxType::Snow);
    parent2.addTrustedPlayer(333);

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    FoxEntity* babyFox = dynamic_cast<FoxEntity*>(baby.get());
    ASSERT_NE(babyFox, nullptr);

    // MC 1.16.5: 幼狐继承父母的信任玩家，但最多只能有2个信任玩家
    // 继承顺序：先添加 parent1 的信任玩家，再添加 parent2 的信任玩家
    // parent1 有 [111, 222]，parent2 有 [333]
    // 添加 parent1 后：[111, 222]
    // 添加 parent2 后：由于 MAX=2，会替换最早的 (111)，结果为 [222, 333]
    EXPECT_EQ(babyFox->getTrustedPlayers().size(), 2u);  // 最多2个信任玩家
    EXPECT_TRUE(babyFox->trusts(222));  // 来自 parent1
    EXPECT_TRUE(babyFox->trusts(333));  // 来自 parent2
    EXPECT_FALSE(babyFox->trusts(111));  // 被替换掉了
}

TEST_F(FoxEntityTest, SpawnBaby_PositionIsSet) {
    FoxEntity parent(LegacyEntityType::Unknown, 1);
    parent.setWorld(&m_world);
    parent.setPosition(100.0f, 64.0f, -50.0f);
    parent.setFoxType(FoxEntity::FoxType::Red);

    FoxEntity partner(LegacyEntityType::Unknown, 2);
    partner.setFoxType(FoxEntity::FoxType::Snow);

    auto baby = parent.spawnBaby(parent);
    ASSERT_NE(baby, nullptr);

    // 幼狐应该在父母附近
    EXPECT_NEAR(baby->x(), 100.0f, 2.0f);
    EXPECT_NEAR(baby->y(), 64.0f, 2.0f);
    EXPECT_NEAR(baby->z(), -50.0f, 2.0f);
}

// ========== 属性测试 ==========

TEST_F(FoxEntityTest, Attributes_HasCorrectBaseValues) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    // MC 1.16.5: 狐狸生命值为 10
    EXPECT_DOUBLE_EQ(fox.maxHealth(), 10.0);

    // MC 1.16.5: 狐狸移动速度为 0.3
    EXPECT_DOUBLE_EQ(fox.getAttributeValue("generic.movement_speed", 0.0), 0.3);
}

// ========== 尺寸测试 ==========
// 注意：FoxEntity 使用 AnimalEntity 的默认尺寸，不在此测试尺寸
// 因为 AnimalEntity 的默认尺寸可能随实现变化

TEST_F(FoxEntityTest, EyeHeight_DifferentForChildAndAdult) {
    FoxEntity adultFox(LegacyEntityType::Unknown, 1);
    adultFox.setChild(false);

    FoxEntity childFox(LegacyEntityType::Unknown, 2);
    childFox.setChild(true);

    // 成体眼睛高度 0.4，幼体 0.2
    EXPECT_FLOAT_EQ(adultFox.eyeHeight(), 0.4f);
    EXPECT_FLOAT_EQ(childFox.eyeHeight(), 0.2f);
}

// ========== 睡眠状态测试 ==========

TEST_F(FoxEntityTest, SleepState_DefaultNotSleeping) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);
    EXPECT_FALSE(fox.isSleeping());
}

TEST_F(FoxEntityTest, SleepState_CanSetSleeping) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    fox.setSleeping(true);
    EXPECT_TRUE(fox.isSleeping());

    fox.setSleeping(false);
    EXPECT_FALSE(fox.isSleeping());
}

// ========== 叼物品测试 ==========

TEST_F(FoxEntityTest, HeldItem_DefaultNotHolding) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);
    EXPECT_FALSE(fox.isHoldingItem());
    EXPECT_EQ(fox.getHeldItem(), nullptr);
}

TEST_F(FoxEntityTest, HeldItem_CanSetAndClear) {
    FoxEntity fox(LegacyEntityType::Unknown, 1);

    auto item = std::make_unique<ItemStack>(Items::SWEET_BERRIES, 1);
    fox.setHeldItem(std::move(item));

    EXPECT_TRUE(fox.isHoldingItem());
    EXPECT_NE(fox.getHeldItem(), nullptr);
    EXPECT_EQ(fox.getHeldItem()->getItem(), Items::SWEET_BERRIES);

    fox.dropHeldItem();
    EXPECT_FALSE(fox.isHoldingItem());
    EXPECT_EQ(fox.getHeldItem(), nullptr);
}

} // namespace
} // namespace mc
