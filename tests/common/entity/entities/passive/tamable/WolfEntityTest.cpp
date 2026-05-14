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
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
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
 * @brief 狼实体测试用世界
 *
 * 提供最小化测试环境用于狼实体功能测试
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

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
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
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    // 骨头是驯服狼的唯一物品
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_TRUE(wolf.isTameItem(boneStack));
}

TEST_F(WolfEntityTestFixture, IsTameItem_Meat_ReturnsFalse)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

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
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::PORKCHOP, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedPorkchop_ReturnsTrue)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::COOKED_PORKCHOP, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Beef_ReturnsTrue)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::BEEF, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedBeef_ReturnsTrue)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::COOKED_BEEF, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Chicken_ReturnsTrue)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::CHICKEN, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedChicken_ReturnsTrue)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::COOKED_CHICKEN, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Rabbit_ReturnsTrue)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::RABBIT, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedRabbit_ReturnsTrue)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::COOKED_RABBIT, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_Mutton_ReturnsTrue)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::MUTTON, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsBreedingItem_CookedMutton_ReturnsTrue)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

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
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::ROTTEN_FLESH, 1);
    EXPECT_TRUE(wolf.isBreedingItem(stack));
}

TEST_F(WolfEntityTestFixture, IsFoodItem_RottenFlesh_ReturnsTrue)
{
    // 狼的食物（用于治疗）与繁殖物品相同
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack stack(Items::ROTTEN_FLESH, 1);
    EXPECT_TRUE(wolf.isFoodItem(stack));
}

// ============================================================================
// 非肉类物品测试
// ============================================================================

TEST_F(WolfEntityTestFixture, IsBreedingItem_NonMeat_ReturnsFalse)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

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
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    // 骨头只能驯服，不能繁殖
    ItemStack boneStack(Items::BONE, 1);
    EXPECT_FALSE(wolf.isBreedingItem(boneStack));
}

// ============================================================================
// 空物品测试
// ============================================================================

TEST_F(WolfEntityTestFixture, IsBreedingItem_EmptyStack_ReturnsFalse)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(wolf.isBreedingItem(emptyStack));
}

TEST_F(WolfEntityTestFixture, IsTameItem_EmptyStack_ReturnsFalse)
{
    WolfEntity wolf(LegacyEntityType::Unknown, 0);

    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(wolf.isTameItem(emptyStack));
}

// ============================================================================
// 生成幼体测试
// ============================================================================

TEST_F(WolfEntityTestFixture, SpawnBaby_CreatesChildWolf)
{
    WolfEntity parent1(LegacyEntityType::Unknown, 0);
    WolfEntity parent2(LegacyEntityType::Unknown, 0);

    auto baby = parent1.spawnBaby(parent2);
    ASSERT_NE(baby, nullptr);

    // 验证是狼实体
    auto* babyWolf = dynamic_cast<WolfEntity*>(baby.get());
    EXPECT_NE(babyWolf, nullptr);

    // 验证是幼体
    EXPECT_TRUE(baby->isChild());
}

} // namespace
} // namespace mc
