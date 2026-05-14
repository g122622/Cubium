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
#include "common/entity/entities/passive/tamable/OcelotEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 豹猫实体测试用世界
 *
 * 提供最小化测试环境用于豹猫实体功能测试
 */
class OcelotTestWorld final : public test::BaseTestWorld {
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
        throw std::runtime_error("OcelotTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("OcelotTestWorld::tickManager not implemented");
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class OcelotEntityTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    OcelotTestWorld m_world;
};

// ============================================================================
// 繁殖物品测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, IsBreedingItem_Cod_ReturnsTrue)
{
    // MC 1.16.5: 豹猫使用生鳕鱼繁殖
    // BREEDING_ITEMS = Ingredient.fromItems(Items.COD, Items.SALMON)
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ItemStack codStack(Items::COD, 1);
    EXPECT_TRUE(ocelot.isBreedingItem(codStack));
}

TEST_F(OcelotEntityTestFixture, IsBreedingItem_Salmon_ReturnsTrue)
{
    // MC 1.16.5: 豹猫使用生鲑鱼繁殖
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ItemStack salmonStack(Items::SALMON, 1);
    EXPECT_TRUE(ocelot.isBreedingItem(salmonStack));
}

TEST_F(OcelotEntityTestFixture, IsBreedingItem_CookedCod_ReturnsFalse)
{
    // 熟鱼不能用于繁殖
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ItemStack cookedCodStack(Items::COOKED_COD, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(cookedCodStack));
}

TEST_F(OcelotEntityTestFixture, IsBreedingItem_CookedSalmon_ReturnsFalse)
{
    // 熟鲑鱼不能用于繁殖
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ItemStack cookedSalmonStack(Items::COOKED_SALMON, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(cookedSalmonStack));
}

// ============================================================================
// 非鱼类物品测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, IsBreedingItem_NonFish_ReturnsFalse)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    // 小麦不能用于豹猫繁殖
    ItemStack wheatStack(Items::WHEAT, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(wheatStack));

    // 胡萝卜不能用于豹猫繁殖
    ItemStack carrotStack(Items::CARROT, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(carrotStack));

    // 骨头不能用于豹猫繁殖
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(boneStack));

    // 生猪肉不能用于豹猫繁殖
    ItemStack porkchopStack(Items::PORKCHOP, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(porkchopStack));

    // 生牛肉不能用于豹猫繁殖
    ItemStack beefStack(Items::BEEF, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(beefStack));
}

// ============================================================================
// 空物品测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, IsBreedingItem_EmptyStack_ReturnsFalse)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(ocelot.isBreedingItem(emptyStack));
}

TEST_F(OcelotEntityTestFixture, IsBreedingItem_NullItem_ReturnsFalse)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ItemStack nullStack(nullptr, 1);
    EXPECT_FALSE(ocelot.isBreedingItem(nullStack));
}

// ============================================================================
// 生成幼体测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, SpawnBaby_CreatesChildOcelot)
{
    OcelotEntity parent1(LegacyEntityType::Unknown, 0);
    OcelotEntity parent2(LegacyEntityType::Unknown, 0);

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    // 验证是豹猫实体
    auto* babyOcelot = dynamic_cast<OcelotEntity*>(baby.get());
    EXPECT_NE(babyOcelot, nullptr);

    // 验证是幼体
    EXPECT_TRUE(baby->isChild());
}

TEST_F(OcelotEntityTestFixture, SpawnBaby_PositionSetCorrectly)
{
    OcelotEntity parent(LegacyEntityType::Unknown, 0);
    parent.setPosition(100.0, 64.0, -50.0);

    auto baby = parent.spawnBaby(parent);
    ASSERT_NE(baby, nullptr);

    // 验证位置继承自父体
    EXPECT_FLOAT_EQ(baby->x(), 100.0f);
    EXPECT_FLOAT_EQ(baby->y(), 64.0f);
    EXPECT_FLOAT_EQ(baby->z(), -50.0f);
}

TEST_F(OcelotEntityTestFixture, SpawnBaby_CreatesNewEntity)
{
    OcelotEntity parent1(LegacyEntityType::Unknown, 0);
    OcelotEntity parent2(LegacyEntityType::Unknown, 0);

    auto baby1 = parent1.spawnBaby(parent2);
    auto baby2 = parent1.spawnBaby(parent2);

    // 每次调用应该创建新的实体
    ASSERT_NE(baby1, nullptr);
    ASSERT_NE(baby2, nullptr);
    EXPECT_NE(baby1.get(), baby2.get());
}

// ============================================================================
// 信任系统测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, TrustSystem_NotTrustingInitially)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    EXPECT_FALSE(ocelot.isTrusting());
    EXPECT_EQ(ocelot.getTrustingPlayerId(), 0u);
}

TEST_F(OcelotEntityTestFixture, TrustSystem_CanSetTrusting)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ocelot.setTrusting(true);
    EXPECT_TRUE(ocelot.isTrusting());

    ocelot.setTrusting(false);
    EXPECT_FALSE(ocelot.isTrusting());
}

TEST_F(OcelotEntityTestFixture, TrustSystem_CanTrustPlayer)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ocelot.setPlayerTrust(12345, true);
    EXPECT_TRUE(ocelot.trustsPlayer(12345));
    EXPECT_EQ(ocelot.getTrustingPlayerId(), 12345u);
}

TEST_F(OcelotEntityTestFixture, TrustSystem_DoesNotTrustOtherPlayers)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ocelot.setPlayerTrust(12345, true);
    EXPECT_FALSE(ocelot.trustsPlayer(67890));
}

TEST_F(OcelotEntityTestFixture, TrustSystem_CannotChangeTrustOnceSet)
{
    // 一旦建立信任，不能更改为其他玩家
    // 参考 MC 1.16.5: setPlayerTrust 只在 !m_trusting 时设置
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ocelot.setPlayerTrust(12345, true);
    EXPECT_EQ(ocelot.getTrustingPlayerId(), 12345u);

    // 尝试更改为其他玩家应该无效
    ocelot.setPlayerTrust(67890, true);
    EXPECT_EQ(ocelot.getTrustingPlayerId(), 12345u); // 仍然是第一个玩家
}

// ============================================================================
// 逃跑状态测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, Fleeing_CanSetFleeingState)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    EXPECT_FALSE(ocelot.isFleeing());

    ocelot.setFleeing(true);
    EXPECT_TRUE(ocelot.isFleeing());

    ocelot.setFleeing(false);
    EXPECT_FALSE(ocelot.isFleeing());
}

// ============================================================================
// 豹猫类型测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, OcelotType_DefaultIsWild)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    EXPECT_EQ(ocelot.getOcelotType(), OcelotEntity::OcelotType::Wild);
}

TEST_F(OcelotEntityTestFixture, OcelotType_CanSetType)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    ocelot.setOcelotType(OcelotEntity::OcelotType::Tabby);
    EXPECT_EQ(ocelot.getOcelotType(), OcelotEntity::OcelotType::Tabby);

    ocelot.setOcelotType(OcelotEntity::OcelotType::Siamese);
    EXPECT_EQ(ocelot.getOcelotType(), OcelotEntity::OcelotType::Siamese);
}

// ============================================================================
// 属性测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, Attributes_HasCorrectBaseValues)
{
    OcelotEntity ocelot(LegacyEntityType::Unknown, 0);

    // MC 1.16.5: 豹猫生命值为 10
    EXPECT_DOUBLE_EQ(ocelot.maxHealth(), 10.0);

    // MC 1.16.5: 豹猫移动速度为 0.3
    EXPECT_DOUBLE_EQ(ocelot.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0), 0.3);
}

// ============================================================================
// 眼睛高度测试
// ============================================================================

TEST_F(OcelotEntityTestFixture, EyeHeight_AdultIsHigher)
{
    OcelotEntity adult(LegacyEntityType::Unknown, 0);
    adult.setChild(false);

    EXPECT_FLOAT_EQ(adult.eyeHeight(), 0.6f);
}

TEST_F(OcelotEntityTestFixture, EyeHeight_ChildIsLower)
{
    OcelotEntity child(LegacyEntityType::Unknown, 0);
    child.setChild(true);

    EXPECT_FLOAT_EQ(child.eyeHeight(), 0.3f);
}

} // namespace
} // namespace mc
