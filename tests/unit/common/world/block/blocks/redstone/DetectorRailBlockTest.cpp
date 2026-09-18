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

#include "common/world/block/blocks/redstone/DetectorRailBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include <map>
#include <memory>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;
using namespace mc::entity;
using namespace mc::block_registry;

namespace {

/**
 * @brief 按资源路径懒注册测试用物品
 */
Item* ensureTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }
    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}

/**
 * @brief 支持 DetectorRailBlock 测试的世界
 *
 * 提供 getBlockState、setBlockState、getEntitiesInAABB 的最小化实现。
 */
class DetectorRailTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

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
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& aabb, const Entity* /*except*/) const override
    {
        std::vector<Entity*> result;
        for (auto* entity : m_entities) {
            if (entity == nullptr || entity->isRemoved()) {
                continue;
            }
            // 检查实体中心点是否在 AABB 内
            f64 ex = static_cast<f64>(entity->x());
            f64 ey = static_cast<f64>(entity->y());
            f64 ez = static_cast<f64>(entity->z());
            if (aabb.contains(Vector3(static_cast<f32>(ex), static_cast<f32>(ey), static_cast<f32>(ez)))) {
                result.push_back(entity);
            }
        }
        return result;
    }

    void updateNeighbors(const BlockPos& /*pos*/, Block& /*sourceBlock*/) override
    {
        // 测试中不需要真正更新邻居
    }

    /**
     * @brief 在指定位置放置方块状态
     */
    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        }
    }

    /**
     * @brief 添加实体到世界中
     */
    void addEntity(Entity* entity) { m_entities.push_back(entity); }

    /**
     * @brief 清空方块和实体
     */
    void clear()
    {
        m_blocks.clear();
        m_entities.clear();
    }

private:
    std::map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<Entity*> m_entities;
};

} // namespace

// ============================================================================
// DetectorRailBlock 比较器信号测试
// ============================================================================

class DetectorRailComparatorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    void TearDown() override { m_world.clear(); }

    DetectorRailTestWorld m_world;
};

// ========== hasComparatorInputOverride 测试 ==========

TEST_F(DetectorRailComparatorTest, HasComparatorInputOverride_ReturnsTrue)
{
    // 探测铁轨始终有比较器信号覆盖
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& defaultState = RedstoneBlocks::DETECTOR_RAIL->defaultState();
    EXPECT_TRUE(RedstoneBlocks::DETECTOR_RAIL->hasComparatorInputOverride(defaultState));
}

TEST_F(DetectorRailComparatorTest, HasComparatorInputOverride_PoweredState_ReturnsTrue)
{
    // 无论是否充能，hasComparatorInputOverride 都返回 true
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& poweredState =
        RedstoneBlocks::DETECTOR_RAIL->defaultState().with(DetectorRailBlock::POWERED(), true);
    EXPECT_TRUE(RedstoneBlocks::DETECTOR_RAIL->hasComparatorInputOverride(poweredState));
}

// ========== getComparatorInputOverride 未激活测试 ==========

TEST_F(DetectorRailComparatorTest, UnpoweredRail_ReturnsZero)
{
    // 未激活的探测铁轨返回0
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& unpoweredState = RedstoneBlocks::DETECTOR_RAIL->defaultState();
    // 确保 POWERED=false
    EXPECT_FALSE(DetectorRailBlock::isPowered(unpoweredState));

    BlockPos pos(5, 64, 10);
    m_world.setBlockAt(pos, &unpoweredState);

    i32 signal = RedstoneBlocks::DETECTOR_RAIL->getComparatorInputOverride(unpoweredState, m_world, pos);
    EXPECT_EQ(signal, 0);
}

TEST_F(DetectorRailComparatorTest, UnpoweredRail_WithMinecart_ReturnsZero)
{
    // 即使有矿车，未激活的探测铁轨也返回0
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& unpoweredState = RedstoneBlocks::DETECTOR_RAIL->defaultState();

    BlockPos pos(5, 64, 10);
    m_world.setBlockAt(pos, &unpoweredState);

    // 添加一个满的箱子矿车
    ChestMinecartEntity chest(EntityInstanceId(1), mc::test::testEcsRegistry());
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    for (i32 i = 0; i < ChestMinecartEntity::INVENTORY_SIZE; ++i) {
        chest.setInventoryItem(i, ItemStack(*diamond, 64));
    }
    chest.setPosition(5.5f, 64.0f, 10.5f);
    m_world.addEntity(&chest);

    i32 signal = RedstoneBlocks::DETECTOR_RAIL->getComparatorInputOverride(unpoweredState, m_world, pos);
    EXPECT_EQ(signal, 0);
}

// ========== getComparatorInputOverride 激活且无矿车测试 ==========

TEST_F(DetectorRailComparatorTest, PoweredRail_NoMinecart_ReturnsZero)
{
    // 激活的探测铁轨没有矿车时返回0
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& poweredState =
        RedstoneBlocks::DETECTOR_RAIL->defaultState().with(DetectorRailBlock::POWERED(), true);
    EXPECT_TRUE(DetectorRailBlock::isPowered(poweredState));

    BlockPos pos(5, 64, 10);
    m_world.setBlockAt(pos, &poweredState);

    i32 signal = RedstoneBlocks::DETECTOR_RAIL->getComparatorInputOverride(poweredState, m_world, pos);
    EXPECT_EQ(signal, 0);
}

// ========== getComparatorInputOverride 容器矿车测试 ==========

TEST_F(DetectorRailComparatorTest, PoweredRail_ChestMinecart_ReturnsContainerSignal)
{
    // 激活的探测铁轨上有箱子矿车，返回容器填充信号
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& poweredState =
        RedstoneBlocks::DETECTOR_RAIL->defaultState().with(DetectorRailBlock::POWERED(), true);

    BlockPos pos(5, 64, 10);
    m_world.setBlockAt(pos, &poweredState);

    ChestMinecartEntity chest(EntityInstanceId(1), mc::test::testEcsRegistry());
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    // 放一个物品，信号应为1
    chest.setInventoryItem(0, ItemStack(*diamond, 1));
    chest.setPosition(5.5f, 64.0f, 10.5f);
    m_world.addEntity(&chest);

    i32 signal = RedstoneBlocks::DETECTOR_RAIL->getComparatorInputOverride(poweredState, m_world, pos);
    EXPECT_GT(signal, 0);
    EXPECT_LE(signal, 15);
}

TEST_F(DetectorRailComparatorTest, PoweredRail_FullChestMinecart_ReturnsFifteen)
{
    // 满的箱子矿车返回15
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& poweredState =
        RedstoneBlocks::DETECTOR_RAIL->defaultState().with(DetectorRailBlock::POWERED(), true);

    BlockPos pos(5, 64, 10);
    m_world.setBlockAt(pos, &poweredState);

    ChestMinecartEntity chest(EntityInstanceId(1), mc::test::testEcsRegistry());
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    for (i32 i = 0; i < ChestMinecartEntity::INVENTORY_SIZE; ++i) {
        chest.setInventoryItem(i, ItemStack(*diamond, 64));
    }
    chest.setPosition(5.5f, 64.0f, 10.5f);
    m_world.addEntity(&chest);

    i32 signal = RedstoneBlocks::DETECTOR_RAIL->getComparatorInputOverride(poweredState, m_world, pos);
    EXPECT_EQ(signal, 15);
}

// ========== getComparatorInputOverride 命令方块矿车优先级测试 ==========

TEST_F(DetectorRailComparatorTest, PoweredRail_CommandBlockMinecart_PriorityOverContainer)
{
    // 命令方块矿车优先于容器矿车
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& poweredState =
        RedstoneBlocks::DETECTOR_RAIL->defaultState().with(DetectorRailBlock::POWERED(), true);

    BlockPos pos(5, 64, 10);
    m_world.setBlockAt(pos, &poweredState);

    // 同时放置命令方块矿车和箱子矿车
    CommandBlockMinecartEntity command(EntityInstanceId(1), mc::test::testEcsRegistry());
    command.setSuccessCount(7);
    command.setPosition(5.5f, 64.0f, 10.5f);
    m_world.addEntity(&command);

    ChestMinecartEntity chest(EntityInstanceId(2), mc::test::testEcsRegistry());
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    for (i32 i = 0; i < ChestMinecartEntity::INVENTORY_SIZE; ++i) {
        chest.setInventoryItem(i, ItemStack(*diamond, 64));
    }
    chest.setPosition(5.4f, 64.0f, 10.4f);
    m_world.addEntity(&chest);

    i32 signal = RedstoneBlocks::DETECTOR_RAIL->getComparatorInputOverride(poweredState, m_world, pos);
    // 命令方块矿车的信号为7，优先于箱子矿车的15
    EXPECT_EQ(signal, 7);
}

// ========== getComparatorInputOverride 普通矿车返回0 ==========

TEST_F(DetectorRailComparatorTest, PoweredRail_RideableMinecart_ReturnsZero)
{
    // 普通矿车不产生比较器信号，返回0（非15，与MC原版一致）
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& poweredState =
        RedstoneBlocks::DETECTOR_RAIL->defaultState().with(DetectorRailBlock::POWERED(), true);

    BlockPos pos(5, 64, 10);
    m_world.setBlockAt(pos, &poweredState);

    RideableMinecartEntity rideable(EntityInstanceId(1), mc::test::testEcsRegistry());
    rideable.setPosition(5.5f, 64.0f, 10.5f);
    m_world.addEntity(&rideable);

    i32 signal = RedstoneBlocks::DETECTOR_RAIL->getComparatorInputOverride(poweredState, m_world, pos);
    EXPECT_EQ(signal, 0);
}

TEST_F(DetectorRailComparatorTest, PoweredRail_TNTMinecart_ReturnsZero)
{
    // TNT矿车不产生比较器信号，返回0
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& poweredState =
        RedstoneBlocks::DETECTOR_RAIL->defaultState().with(DetectorRailBlock::POWERED(), true);

    BlockPos pos(5, 64, 10);
    m_world.setBlockAt(pos, &poweredState);

    TNTMinecartEntity tnt(EntityInstanceId(1), mc::test::testEcsRegistry());
    tnt.setPosition(5.5f, 64.0f, 10.5f);
    m_world.addEntity(&tnt);

    i32 signal = RedstoneBlocks::DETECTOR_RAIL->getComparatorInputOverride(poweredState, m_world, pos);
    EXPECT_EQ(signal, 0);
}

TEST_F(DetectorRailComparatorTest, PoweredRail_FurnaceMinecart_ReturnsZero)
{
    // 熔炉矿车不产生比较器信号，返回0
    ASSERT_NE(RedstoneBlocks::DETECTOR_RAIL, nullptr);
    const BlockState& poweredState =
        RedstoneBlocks::DETECTOR_RAIL->defaultState().with(DetectorRailBlock::POWERED(), true);

    BlockPos pos(5, 64, 10);
    m_world.setBlockAt(pos, &poweredState);

    FurnaceMinecartEntity furnace(EntityInstanceId(1), mc::test::testEcsRegistry());
    furnace.setPosition(5.5f, 64.0f, 10.5f);
    m_world.addEntity(&furnace);

    i32 signal = RedstoneBlocks::DETECTOR_RAIL->getComparatorInputOverride(poweredState, m_world, pos);
    EXPECT_EQ(signal, 0);
}
