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
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/passive/basic/ChickenEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

class ChickenTestWorld final : public mc::test::BaseTestWorld {
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

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (auto* itemEntity = dynamic_cast<ItemEntity*>(entity.get())) {
            m_spawnedStacks.push_back(itemEntity->getItemStack());
        }

        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] const std::vector<ItemStack>& spawnedStacks() const { return m_spawnedStacks; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ChickenTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ChickenTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<ItemStack> m_spawnedStacks;
};

class ChickenEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        // 注册原版实体类型：ChickenEntity::tick 下蛋链路经 EntityRegistry::getType(ITEM) 取物品实体
        // 工厂，未注册则 resetEggTimer 静默返回不下蛋（与 ItemDropHelperTest/BlockDropHandlerTest 同范式）。
        entity::VanillaEntities::registerAll();
    }

    ChickenTestWorld m_world;
};

TEST_F(ChickenEntityTest, Tick_EmitsEggItemAfterTimerExpires)
{
    ChickenEntity chicken(EntityInstanceId(1), mc::test::testEcsRegistry());
    chicken.setWorld(&m_world);
    chicken.setPosition(0.5f, 64.0f, 0.5f);

    const i32 eggTimer = chicken.getEggTimer();
    for (i32 i = 0; i < eggTimer; ++i) {
        chicken.tick();
    }

    ASSERT_EQ(m_world.spawnedStacks().size(), 1u);
    EXPECT_EQ(m_world.spawnedStacks().front().getItem(), Items::EGG);
    EXPECT_EQ(m_world.spawnedStacks().front().getCount(), 1);
    EXPECT_GT(chicken.getEggTimer(), 0);
}

} // namespace
} // namespace mc
