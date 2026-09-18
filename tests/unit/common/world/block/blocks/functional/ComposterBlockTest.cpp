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

#include "world/block/blocks/functional/ComposterBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "entity/inventory/ISidedInventory.hpp"
#include "entity/inventory/ISidedInventoryProvider.hpp"
#include "item/Items.hpp"
#include "world/block/blocks/functional/CompostableItems.hpp"
#include <gtest/gtest.h>

#include <map>
#include <memory>

using namespace mc;
using namespace mc::blocks;

// ========== ComposterBlock 形状测试 ==========

class ComposterBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        composter_ = std::make_unique<ComposterBlock>(BlockProperties(Material::WOOD).hardness(0.6f).resistance(0.6f));
    }

    std::unique_ptr<ComposterBlock> composter_;
};

// ========== 基本属性测试 ==========

TEST_F(ComposterBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(composter_, nullptr);
}

TEST_F(ComposterBlockTest, DefaultState_HasCorrectLevel)
{
    const auto& state = composter_->defaultState();
    EXPECT_EQ(ComposterBlock::getLevel(state), 0);
}

TEST_F(ComposterBlockTest, IsOpaque_ReturnsTrue)
{
    const auto& state = composter_->defaultState();
    // MC Java 中 ComposterBlock 未重写 isOpaque，默认为 true
    // 即使有空腔，方块本身是不透明的（阻挡光线传播）
    EXPECT_TRUE(composter_->isOpaque(state));
}

// ========== 渲染形状测试 ==========

TEST_F(ComposterBlockTest, GetShape_Level0_NotEmpty)
{
    // 等级0：底板2像素 + 四面墙壁
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& shape = composter_->getShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Level 0 shape should not be empty";
}

TEST_F(ComposterBlockTest, GetShape_Level0_NotFullBlock)
{
    // 等级0不是完整方块，应该是有空腔的外壁形状
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& shape = composter_->getShape(state);
    EXPECT_FALSE(shape.isFullBlock()) << "Level 0 shape should NOT be a full block (it has a hollow interior)";
}

TEST_F(ComposterBlockTest, GetShape_AllLevels_NotEmpty)
{
    // 所有等级的形状都应非空
    for (i32 level = 0; level <= 8; ++level) {
        auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), level);
        const auto& shape = composter_->getShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Level " << level << " shape should not be empty";
    }
}

TEST_F(ComposterBlockTest, GetShape_Level7And8_AreIdentical)
{
    // MC Java: avoxelshape[8] = avoxelshape[7]
    auto state7 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7);
    auto state8 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);

    const auto& shape7 = composter_->getShape(state7);
    const auto& shape8 = composter_->getShape(state8);

    // 两个形状的包围盒应完全相同
    auto boxes7 = shape7.getWorldBoxes(0, 0, 0);
    auto boxes8 = shape8.getWorldBoxes(0, 0, 0);
    ASSERT_EQ(boxes7.size(), boxes8.size()) << "Level 7 and 8 should have same number of boxes";

    for (size_t i = 0; i < boxes7.size(); ++i) {
        EXPECT_FLOAT_EQ(boxes7[i].minX, boxes8[i].minX) << "Box " << i << " minX differs between level 7 and 8";
        EXPECT_FLOAT_EQ(boxes7[i].minY, boxes8[i].minY) << "Box " << i << " minY differs between level 7 and 8";
        EXPECT_FLOAT_EQ(boxes7[i].minZ, boxes8[i].minZ) << "Box " << i << " minZ differs between level 7 and 8";
        EXPECT_FLOAT_EQ(boxes7[i].maxX, boxes8[i].maxX) << "Box " << i << " maxX differs between level 7 and 8";
        EXPECT_FLOAT_EQ(boxes7[i].maxY, boxes8[i].maxY) << "Box " << i << " maxY differs between level 7 and 8";
        EXPECT_FLOAT_EQ(boxes7[i].maxZ, boxes8[i].maxZ) << "Box " << i << " maxZ differs between level 7 and 8";
    }
}

TEST_F(ComposterBlockTest, GetShape_HasFiveBoxes)
{
    // 外壁形状由5部分组成：底板 + 4面墙
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& shape = composter_->getShape(state);
    auto boxes = shape.getWorldBoxes(0, 0, 0);
    // 底板 + 北墙 + 南墙 + 西墙 + 东墙 = 5 个AABB
    EXPECT_EQ(boxes.size(), 5u) << "Composter shape should consist of 5 boxes (base + 4 walls)";
}

TEST_F(ComposterBlockTest, GetShape_Level0_BaseHeight)
{
    // 等级0：fillHeightPixels = max(2, 1+0*2) = 2，底板高度 = 2/16
    constexpr f32 P = 1.0f / 16.0f;
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& shape = composter_->getShape(state);
    auto boxes = shape.getWorldBoxes(0, 0, 0);

    // 底板应为 (0, 0, 0) -> (1, 2P, 1)
    bool foundBase = false;
    for (const auto& box : boxes) {
        if (box.minX == 0.0f && box.minY == 0.0f && box.minZ == 0.0f && box.maxX == 1.0f && box.maxZ == 1.0f) {
            EXPECT_FLOAT_EQ(box.maxY, 2.0f * P) << "Level 0 base should be 2 pixels tall";
            foundBase = true;
            break;
        }
    }
    EXPECT_TRUE(foundBase) << "Level 0 shape should have a base box at y=0";
}

TEST_F(ComposterBlockTest, GetShape_Level0_WallHeight)
{
    // 等级0：墙壁从 y=2P 延伸到 y=1.0
    constexpr f32 P = 1.0f / 16.0f;
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& shape = composter_->getShape(state);
    auto boxes = shape.getWorldBoxes(0, 0, 0);

    // 检查北墙：(0, 2P, 0) -> (1, 1, 2P)
    bool foundNorthWall = false;
    for (const auto& box : boxes) {
        if (box.minX == 0.0f && box.minZ == 0.0f && box.maxX == 1.0f && box.maxZ == 2.0f * P) {
            EXPECT_FLOAT_EQ(box.minY, 2.0f * P) << "North wall should start at y=2P";
            EXPECT_FLOAT_EQ(box.maxY, 1.0f) << "North wall should reach y=1.0";
            foundNorthWall = true;
            break;
        }
    }
    EXPECT_TRUE(foundNorthWall) << "Level 0 shape should have a north wall";
}

TEST_F(ComposterBlockTest, GetShape_HigherLevelsHaveHigherFill)
{
    // 更高等级的底板更高，空心区域更小
    // 验证等级7的底板高度 > 等级0的底板高度
    constexpr f32 P = 1.0f / 16.0f;
    auto state0 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    auto state7 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7);

    const auto& shape0 = composter_->getShape(state0);
    const auto& shape7 = composter_->getShape(state7);

    auto boxes0 = shape0.getWorldBoxes(0, 0, 0);
    auto boxes7 = shape7.getWorldBoxes(0, 0, 0);

    // 找到底板（ minX=0, minZ=0, maxX=1, maxZ=1 的 box）
    f32 baseY0 = 0.0f;
    f32 baseY7 = 0.0f;
    for (const auto& box : boxes0) {
        if (box.minX == 0.0f && box.minZ == 0.0f && box.maxX == 1.0f && box.maxZ == 1.0f) {
            baseY0 = box.maxY;
        }
    }
    for (const auto& box : boxes7) {
        if (box.minX == 0.0f && box.minZ == 0.0f && box.maxX == 1.0f && box.maxZ == 1.0f) {
            baseY7 = box.maxY;
        }
    }

    // 等级7：fillHeightPixels = max(2, 1+7*2) = 15，底板高度 = 15/16
    // 等级0：fillHeightPixels = max(2, 1+0*2) = 2，底板高度 = 2/16
    EXPECT_GT(baseY7, baseY0) << "Level 7 should have a higher base than level 0";
    EXPECT_FLOAT_EQ(baseY0, 2.0f * P) << "Level 0 base height should be 2/16";
    EXPECT_FLOAT_EQ(baseY7, 15.0f * P) << "Level 7 base height should be 15/16";
}

// ========== 碰撞形状测试 ==========

TEST_F(ComposterBlockTest, GetCollisionShape_IsLevel0Shape)
{
    // MC Java: getCollisionShape() 始终返回 SHAPES[0]（等级0的外壳形状）
    auto state0 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    auto state5 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 5);
    auto state8 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);

    const auto& collisionShape0 = composter_->getCollisionShape(state0);
    const auto& collisionShape5 = composter_->getCollisionShape(state5);
    const auto& collisionShape8 = composter_->getCollisionShape(state8);

    // 所有等级的碰撞形状应该相同（等于等级0的渲染形状）
    auto boxes0 = collisionShape0.getWorldBoxes(0, 0, 0);
    auto boxes5 = collisionShape5.getWorldBoxes(0, 0, 0);
    auto boxes8 = collisionShape8.getWorldBoxes(0, 0, 0);

    EXPECT_EQ(boxes0.size(), boxes5.size()) << "Collision shape should have same box count across levels";
    EXPECT_EQ(boxes0.size(), boxes8.size()) << "Collision shape should have same box count across levels";

    for (size_t i = 0; i < boxes0.size(); ++i) {
        EXPECT_FLOAT_EQ(boxes0[i].minX, boxes5[i].minX) << "Collision box " << i << " minX differs";
        EXPECT_FLOAT_EQ(boxes0[i].minY, boxes5[i].minY) << "Collision box " << i << " minY differs";
        EXPECT_FLOAT_EQ(boxes0[i].minZ, boxes5[i].minZ) << "Collision box " << i << " minZ differs";
        EXPECT_FLOAT_EQ(boxes0[i].maxX, boxes5[i].maxX) << "Collision box " << i << " maxX differs";
        EXPECT_FLOAT_EQ(boxes0[i].maxY, boxes5[i].maxY) << "Collision box " << i << " maxY differs";
        EXPECT_FLOAT_EQ(boxes0[i].maxZ, boxes5[i].maxZ) << "Collision box " << i << " maxZ differs";
    }
}

TEST_F(ComposterBlockTest, GetCollisionShape_NotFullBlock)
{
    // 碰撞形状不应该是完整方块（有空腔可以站进去）
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    const auto& collisionShape = composter_->getCollisionShape(state);
    EXPECT_FALSE(collisionShape.isFullBlock()) << "Collision shape should NOT be a full block";
}

TEST_F(ComposterBlockTest, GetCollisionShape_EqualsLevel0Shape)
{
    // 碰撞形状应与等级0的渲染形状完全相同
    auto state0 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    auto state5 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 5);

    const auto& shape0 = composter_->getShape(state0);
    const auto& collisionShape5 = composter_->getCollisionShape(state5);

    auto shape0Boxes = shape0.getWorldBoxes(0, 0, 0);
    auto collisionBoxes = collisionShape5.getWorldBoxes(0, 0, 0);

    ASSERT_EQ(shape0Boxes.size(), collisionBoxes.size());
    for (size_t i = 0; i < shape0Boxes.size(); ++i) {
        EXPECT_FLOAT_EQ(shape0Boxes[i].minX, collisionBoxes[i].minX);
        EXPECT_FLOAT_EQ(shape0Boxes[i].minY, collisionBoxes[i].minY);
        EXPECT_FLOAT_EQ(shape0Boxes[i].minZ, collisionBoxes[i].minZ);
        EXPECT_FLOAT_EQ(shape0Boxes[i].maxX, collisionBoxes[i].maxX);
        EXPECT_FLOAT_EQ(shape0Boxes[i].maxY, collisionBoxes[i].maxY);
        EXPECT_FLOAT_EQ(shape0Boxes[i].maxZ, collisionBoxes[i].maxZ);
    }
}

// ========== 比较器输出测试 ==========

TEST_F(ComposterBlockTest, HasComparatorInputOverride)
{
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 5);
    EXPECT_TRUE(composter_->hasComparatorInputOverride(state));
}

// ========== 各等级底板高度验证 ==========

TEST_F(ComposterBlockTest, GetShape_AllLevelsFillHeightMatchesMC)
{
    // MC Java: Block.column(12.0, clamp(1 + level * 2, 2, 16), 16.0)
    // fillHeightPixels = max(2, 1 + level * 2) for level 0-7
    constexpr f32 P = 1.0f / 16.0f;

    i32 expectedFillHeights[] = {2, 3, 5, 7, 9, 11, 13, 15};

    for (i32 level = 0; level < 8; ++level) {
        auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), level);
        const auto& shape = composter_->getShape(state);
        auto boxes = shape.getWorldBoxes(0, 0, 0);

        // 找到底板（覆盖整个 XZ 平面的 box）
        bool foundBase = false;
        for (const auto& box : boxes) {
            if (box.minX == 0.0f && box.minZ == 0.0f && box.maxX == 1.0f && box.maxZ == 1.0f) {
                f32 expectedHeight = static_cast<f32>(expectedFillHeights[level]) * P;
                EXPECT_FLOAT_EQ(box.maxY, expectedHeight)
                    << "Level " << level << " base height should be " << expectedFillHeights[level] << "/16";
                foundBase = true;
                break;
            }
        }
        EXPECT_TRUE(foundBase) << "Level " << level << " should have a base box covering full XZ plane";
    }
}

// ============================================================================
// 测试用世界桩 - 用于 ComposterBlock 容器测试
// ============================================================================

/**
 * @brief ComposterBlock 容器测试用的世界桩
 *
 * 继承 BaseTestWorld，提供可控的方块状态存储、随机数控制和 TickManager。
 */
class ComposterContainerTestWorld : public mc::test::BaseTestWorld {
public:
    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            m_ownedStates.erase(pos);
        } else {
            auto [it, inserted] = m_ownedStates.insert_or_assign(pos, *state);
            m_blocks[pos] = &it->second;
        }
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<ComposterContainerTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    /// 设置随机数种子以控制 nextFloat() 结果
    void setRandomSeed(u64 seed) { m_random.setSeed(seed); }

    /// 检查指定位置的方块状态等级
    [[nodiscard]] i32 getLevelAt(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end() && it->second != nullptr) {
            return ComposterBlock::getLevel(*it->second);
        }
        return -1;
    }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
};

// ============================================================================
// ComposterBlock 容器测试
// ============================================================================

class ComposterContainerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        Items::initialize();
        CompostableItems::initialize();
    }

    void SetUp() override
    {
        composter_ = std::make_unique<ComposterBlock>(BlockProperties(Material::WOOD).hardness(0.6f).resistance(0.6f));
    }

    std::unique_ptr<ComposterBlock> composter_;
    ComposterContainerTestWorld world_;
};

// ========== EmptyContainer 测试（等级 7）==========

TEST_F(ComposterContainerTest, EmptyContainer_SizeIsZero)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_EQ(inv->getContainerSize(), 0);
}

TEST_F(ComposterContainerTest, EmptyContainer_IsEmpty)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_TRUE(inv->isEmpty());
}

TEST_F(ComposterContainerTest, EmptyContainer_GetItemReturnsEmpty)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_TRUE(inv->getItem(0).isEmpty());
    EXPECT_TRUE(inv->getItem(999).isEmpty());
}

TEST_F(ComposterContainerTest, EmptyContainer_SetItemIsNoOp)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    // 设置物品不应崩溃，也不应改变任何状态
    if (Items::APPLE != nullptr) {
        inv->setItem(0, ItemStack(Items::APPLE, 1));
    }
    EXPECT_TRUE(inv->isEmpty());
    EXPECT_EQ(inv->getContainerSize(), 0);
}

TEST_F(ComposterContainerTest, EmptyContainer_RemoveItemReturnsEmpty)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_TRUE(inv->removeItem(0, 1).isEmpty());
    EXPECT_TRUE(inv->removeItemNoUpdate(0).isEmpty());
}

TEST_F(ComposterContainerTest, EmptyContainer_GetSlotsForFaceReturnsEmpty)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_TRUE(inv->getSlotsForFace(Direction::Up).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::Down).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::North).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::South).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::East).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::West).empty());
}

TEST_F(ComposterContainerTest, EmptyContainer_CanInsertItemAlwaysFalse)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    if (Items::APPLE != nullptr) {
        ItemStack stack(Items::APPLE, 1);
        EXPECT_FALSE(inv->canInsertItem(0, stack, Direction::Up));
        EXPECT_FALSE(inv->canInsertItem(0, stack, Direction::Down));
    }
}

TEST_F(ComposterContainerTest, EmptyContainer_CanExtractItemAlwaysFalse)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    if (Items::APPLE != nullptr) {
        ItemStack stack(Items::APPLE, 1);
        EXPECT_FALSE(inv->canExtractItem(0, stack, Direction::Down));
    }
}

TEST_F(ComposterContainerTest, EmptyContainer_ClearAndSetChangedAreNoOp)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    inv->clear();      // 不应崩溃
    inv->setChanged(); // 不应崩溃
    EXPECT_TRUE(inv->isEmpty());
}

// ========== InputContainer 测试（等级 0-6）==========

TEST_F(ComposterContainerTest, InputContainer_SizeIsOne)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_EQ(inv->getContainerSize(), 1);
}

TEST_F(ComposterContainerTest, InputContainer_IsEmptyInitially)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_TRUE(inv->isEmpty());
}

TEST_F(ComposterContainerTest, InputContainer_MaxStackSizeIsOne)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_EQ(inv->getMaxStackSize(), 1);
}

TEST_F(ComposterContainerTest, InputContainer_GetSlotsForFace_OnlyUp)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    auto upSlots = inv->getSlotsForFace(Direction::Up);
    ASSERT_EQ(upSlots.size(), 1u);
    EXPECT_EQ(upSlots[0], 0);

    EXPECT_TRUE(inv->getSlotsForFace(Direction::Down).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::North).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::South).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::East).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::West).empty());
}

TEST_F(ComposterContainerTest, InputContainer_CanInsertItem_UpCompostable)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        ItemStack appleStack(Items::APPLE, 1);
        // 仅从上方可以插入可堆肥物品
        EXPECT_TRUE(inv->canInsertItem(0, appleStack, Direction::Up));
        EXPECT_FALSE(inv->canInsertItem(0, appleStack, Direction::Down));
        EXPECT_FALSE(inv->canInsertItem(0, appleStack, Direction::North));
    }
}

TEST_F(ComposterContainerTest, InputContainer_CanInsertItem_NonCompostable)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::DIAMOND != nullptr) {
        ItemStack diamondStack(Items::DIAMOND, 1);
        // 钻石不可堆肥，从任何方向都不能插入
        EXPECT_FALSE(inv->canInsertItem(0, diamondStack, Direction::Up));
    }
}

TEST_F(ComposterContainerTest, InputContainer_CanExtractItemAlwaysFalse)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        ItemStack appleStack(Items::APPLE, 1);
        // 输入容器不允许提取
        EXPECT_FALSE(inv->canExtractItem(0, appleStack, Direction::Down));
        EXPECT_FALSE(inv->canExtractItem(0, appleStack, Direction::Up));
    }
}

TEST_F(ComposterContainerTest, InputContainer_CanPlaceItem_CompostableItem)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        ItemStack appleStack(Items::APPLE, 1);
        EXPECT_TRUE(inv->canPlaceItem(0, appleStack));
    }
}

TEST_F(ComposterContainerTest, InputContainer_CanPlaceItem_NonCompostableItem)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::DIAMOND != nullptr) {
        ItemStack diamondStack(Items::DIAMOND, 1);
        EXPECT_FALSE(inv->canPlaceItem(0, diamondStack));
    }
}

TEST_F(ComposterContainerTest, InputContainer_RemoveItemReturnsEmpty)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    // 输入容器不允许提取
    EXPECT_TRUE(inv->removeItem(0, 1).isEmpty());
    EXPECT_TRUE(inv->removeItemNoUpdate(0).isEmpty());
}

TEST_F(ComposterContainerTest, InputContainer_SetItemTriggersCompostAndClearsSlot)
{
    // 设置随机种子使 nextFloat() 返回 >= 0.65 的值（堆肥失败）
    // 这样 InputContainer::setChanged() 调用 attemptCompost 后不会改变等级
    // 但无论成功与否，m_item 都会被清空
    const BlockPos pos(5, 10, 15);
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    world_.setBlockAt(pos, &state);

    auto inv = composter_->createInventory(state, world_, pos);
    ASSERT_NE(inv, nullptr);
    EXPECT_TRUE(inv->isEmpty());

    if (Items::APPLE != nullptr) {
        // 设置随机种子以确保确定性行为
        // APPLE 的堆肥概率为 0.65，种子 12345 时第一次 nextFloat 不一定是 >= 0.65
        // 但无论如何，setItem 后槽位都应该被清空（因为 setChanged 会处理）
        inv->setItem(0, ItemStack(Items::APPLE, 1));
        // setChanged() 被调用后，m_item 应该被清空（无论堆肥成功与否）
        EXPECT_TRUE(inv->isEmpty());
        EXPECT_TRUE(inv->getItem(0).isEmpty());
    }
}

TEST_F(ComposterContainerTest, InputContainer_CannotInsertAfterChanged)
{
    // 一旦 setChanged() 被调用（即物品被放入后），canInsertItem 返回 false
    const BlockPos pos(5, 10, 15);
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    world_.setBlockAt(pos, &state);

    auto inv = composter_->createInventory(state, world_, pos);
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        // 第一次插入应该被允许
        EXPECT_TRUE(inv->canInsertItem(0, ItemStack(Items::APPLE, 1), Direction::Up));
        EXPECT_TRUE(inv->canPlaceItem(0, ItemStack(Items::APPLE, 1)));

        // 放入物品，触发 setChanged
        inv->setItem(0, ItemStack(Items::APPLE, 1));

        // 之后不能再次插入
        EXPECT_FALSE(inv->canInsertItem(0, ItemStack(Items::APPLE, 1), Direction::Up));
        EXPECT_FALSE(inv->canPlaceItem(0, ItemStack(Items::APPLE, 1)));
    }
}

TEST_F(ComposterContainerTest, InputContainer_ClearEmptiesSlot)
{
    const BlockPos pos(5, 10, 15);
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    world_.setBlockAt(pos, &state);

    auto inv = composter_->createInventory(state, world_, pos);
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        // 注意：setItem 会触发 setChanged -> attemptCompost -> clear slot
        // 但直接调用 clear() 也可以清空
        inv->clear();
        EXPECT_TRUE(inv->isEmpty());
    }
}

// ========== OutputContainer 测试（等级 8）==========

TEST_F(ComposterContainerTest, OutputContainer_SizeIsOne)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_EQ(inv->getContainerSize(), 1);
}

TEST_F(ComposterContainerTest, OutputContainer_ContainsBoneMealInitially)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::BONE_MEAL != nullptr) {
        EXPECT_FALSE(inv->isEmpty());
        ItemStack item = inv->getItem(0);
        EXPECT_FALSE(item.isEmpty());
        EXPECT_EQ(item.getItem(), Items::BONE_MEAL);
        EXPECT_EQ(item.getCount(), 1);
    }
}

TEST_F(ComposterContainerTest, OutputContainer_MaxStackSizeIsOne)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_EQ(inv->getMaxStackSize(), 1);
}

TEST_F(ComposterContainerTest, OutputContainer_GetSlotsForFace_OnlyDown)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    auto downSlots = inv->getSlotsForFace(Direction::Down);
    ASSERT_EQ(downSlots.size(), 1u);
    EXPECT_EQ(downSlots[0], 0);

    EXPECT_TRUE(inv->getSlotsForFace(Direction::Up).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::North).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::South).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::East).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::West).empty());
}

TEST_F(ComposterContainerTest, OutputContainer_CanInsertItemAlwaysFalse)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        ItemStack appleStack(Items::APPLE, 1);
        EXPECT_FALSE(inv->canInsertItem(0, appleStack, Direction::Up));
        EXPECT_FALSE(inv->canInsertItem(0, appleStack, Direction::Down));
        EXPECT_FALSE(inv->canInsertItem(0, appleStack, Direction::North));
    }
}

TEST_F(ComposterContainerTest, OutputContainer_CanExtractItem_DownBoneMeal)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::BONE_MEAL != nullptr) {
        ItemStack boneMealStack(Items::BONE_MEAL, 1);
        // 仅从下方可以提取骨粉
        EXPECT_TRUE(inv->canExtractItem(0, boneMealStack, Direction::Down));
        EXPECT_FALSE(inv->canExtractItem(0, boneMealStack, Direction::Up));
        EXPECT_FALSE(inv->canExtractItem(0, boneMealStack, Direction::North));
    }
}

TEST_F(ComposterContainerTest, OutputContainer_CanExtractItem_NonBoneMeal)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::DIAMOND != nullptr) {
        ItemStack diamondStack(Items::DIAMOND, 1);
        // 非骨粉物品不能从输出容器提取
        EXPECT_FALSE(inv->canExtractItem(0, diamondStack, Direction::Down));
    }
}

TEST_F(ComposterContainerTest, OutputContainer_RemoveItem_ReturnsBoneMeal)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::BONE_MEAL != nullptr) {
        ItemStack removed = inv->removeItem(0, 1);
        EXPECT_FALSE(removed.isEmpty());
        EXPECT_EQ(removed.getItem(), Items::BONE_MEAL);
        EXPECT_EQ(removed.getCount(), 1);

        // 移除后槽位应该变空
        EXPECT_TRUE(inv->getItem(0).isEmpty());
    }
}

TEST_F(ComposterContainerTest, OutputContainer_RemoveItemNoUpdate_ReturnsBoneMeal)
{
    auto inv = composter_->createInventory(
        composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8), world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::BONE_MEAL != nullptr) {
        ItemStack removed = inv->removeItemNoUpdate(0);
        EXPECT_FALSE(removed.isEmpty());
        EXPECT_EQ(removed.getItem(), Items::BONE_MEAL);
        EXPECT_EQ(removed.getCount(), 1);
    }
}

TEST_F(ComposterContainerTest, OutputContainer_CannotExtractAfterChanged)
{
    // 一旦 setChanged() 被调用（骨粉被提取），canExtractItem 返回 false
    const BlockPos pos(5, 10, 15);
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);
    world_.setBlockAt(pos, &state);

    auto inv = composter_->createInventory(state, world_, pos);
    ASSERT_NE(inv, nullptr);

    if (Items::BONE_MEAL != nullptr) {
        // 提取前可以提取
        EXPECT_TRUE(inv->canExtractItem(0, ItemStack(Items::BONE_MEAL, 1), Direction::Down));

        // 提取骨粉
        inv->removeItem(0, 1);

        // 提取后不能再次提取
        EXPECT_FALSE(inv->canExtractItem(0, ItemStack(Items::BONE_MEAL, 1), Direction::Down));
    }
}

TEST_F(ComposterContainerTest, OutputContainer_RemoveItemTriggersEmpty)
{
    // 当骨粉被提取完（m_item 变空），setChanged() 会调用 ComposterBlock::empty()
    // 这会将方块状态重置为等级 0
    const BlockPos pos(5, 10, 15);
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);
    world_.setBlockAt(pos, &state);

    // 确认世界中方块等级为 8
    EXPECT_EQ(ComposterBlock::getLevel(*world_.getBlockState(pos.x, pos.y, pos.z)), 8);

    auto inv = composter_->createInventory(state, world_, pos);
    ASSERT_NE(inv, nullptr);

    if (Items::BONE_MEAL != nullptr) {
        // 提取骨粉（服务端模式，empty() 会修改方块状态）
        EXPECT_FALSE(world_.isClientSide());
        ItemStack removed = inv->removeItem(0, 1);
        EXPECT_EQ(removed.getItem(), Items::BONE_MEAL);

        // setChanged() 被调用 -> ComposterBlock::empty() -> 方块状态变为等级 0
        // 注意：empty() 会调用 setBlockState，需要验证世界状态改变
        // 但因为 inv 持有的是构造时的 state 引用，且 ComposterBlock::empty 需要 IWorld
        // 我们验证方块是否被更新
        const BlockState* newState = world_.getBlockState(pos.x, pos.y, pos.z);
        if (newState != nullptr) {
            EXPECT_EQ(ComposterBlock::getLevel(*newState), 0);
        }
    }
}

TEST_F(ComposterContainerTest, OutputContainer_ClearTriggersEmpty)
{
    // clear() 清空 m_item 也会触发 setChanged() -> empty()
    const BlockPos pos(5, 10, 15);
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);
    world_.setBlockAt(pos, &state);

    auto inv = composter_->createInventory(state, world_, pos);
    ASSERT_NE(inv, nullptr);

    if (Items::BONE_MEAL != nullptr) {
        inv->clear();
        EXPECT_TRUE(inv->isEmpty());

        // setChanged() 被调用 -> ComposterBlock::empty()
        const BlockState* newState = world_.getBlockState(pos.x, pos.y, pos.z);
        if (newState != nullptr) {
            EXPECT_EQ(ComposterBlock::getLevel(*newState), 0);
        }
    }
}

// ========== createInventory 测试 ==========

TEST_F(ComposterContainerTest, CreateInventory_Level0_ReturnsInputContainer)
{
    for (i32 level = 0; level <= 6; ++level) {
        auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), level);
        auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
        ASSERT_NE(inv, nullptr) << "Level " << level << " should return non-null inventory";
        EXPECT_EQ(inv->getContainerSize(), 1) << "Level " << level << " should return InputContainer (size 1)";
        // 验证是 InputContainer：getSlotsForFace(Up) 返回 {0}
        auto slots = inv->getSlotsForFace(Direction::Up);
        EXPECT_EQ(slots.size(), 1u) << "Level " << level << " InputContainer should have Up slot";
    }
}

TEST_F(ComposterContainerTest, CreateInventory_Level7_ReturnsEmptyContainer)
{
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7);
    auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_EQ(inv->getContainerSize(), 0);
    EXPECT_TRUE(inv->isEmpty());
}

TEST_F(ComposterContainerTest, CreateInventory_Level8_ReturnsOutputContainer)
{
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);
    auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_EQ(inv->getContainerSize(), 1);
    // 验证是 OutputContainer：getSlotsForFace(Down) 返回 {0}
    auto slots = inv->getSlotsForFace(Direction::Down);
    EXPECT_EQ(slots.size(), 1u);
}

TEST_F(ComposterContainerTest, CreateInventory_DifferentLevelsReturnDifferentTypes)
{
    auto state0 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    auto state7 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7);
    auto state8 = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);

    auto inv0 = composter_->createInventory(state0, world_, BlockPos(0, 0, 0));
    auto inv7 = composter_->createInventory(state7, world_, BlockPos(0, 0, 0));
    auto inv8 = composter_->createInventory(state8, world_, BlockPos(0, 0, 0));

    ASSERT_NE(inv0, nullptr);
    ASSERT_NE(inv7, nullptr);
    ASSERT_NE(inv8, nullptr);

    // InputContainer: Up 方向有槽位，Down 方向无槽位
    EXPECT_FALSE(inv0->getSlotsForFace(Direction::Up).empty());
    EXPECT_TRUE(inv0->getSlotsForFace(Direction::Down).empty());

    // EmptyContainer: 所有方向无槽位
    EXPECT_TRUE(inv7->getSlotsForFace(Direction::Up).empty());
    EXPECT_TRUE(inv7->getSlotsForFace(Direction::Down).empty());

    // OutputContainer: Down 方向有槽位，Up 方向无槽位
    EXPECT_TRUE(inv8->getSlotsForFace(Direction::Up).empty());
    EXPECT_FALSE(inv8->getSlotsForFace(Direction::Down).empty());
}

// ========== ISidedInventoryProvider 接口测试 ==========

TEST_F(ComposterContainerTest, ComposterBlock_ImplementsISidedInventoryProvider)
{
    // 验证 ComposterBlock 可以通过 ISidedInventoryProvider 接口使用
    ISidedInventoryProvider* provider = composter_.get();
    ASSERT_NE(provider, nullptr);

    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    auto inv = provider->createInventory(state, world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);
    EXPECT_EQ(inv->getContainerSize(), 1);
}

// ========== 方向性访问综合测试 ==========

TEST_F(ComposterContainerTest, DirectionalAccess_InputContainerOnlyUp)
{
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        ItemStack compostable(Items::APPLE, 1);

        // 从上方可以插入
        EXPECT_TRUE(inv->canInsertItem(0, compostable, Direction::Up));
        // 从其他方向不能插入
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::Down));
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::North));
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::South));
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::East));
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::West));

        // 任何方向都不能提取
        EXPECT_FALSE(inv->canExtractItem(0, compostable, Direction::Down));
        EXPECT_FALSE(inv->canExtractItem(0, compostable, Direction::Up));
        EXPECT_FALSE(inv->canExtractItem(0, compostable, Direction::North));
    }
}

TEST_F(ComposterContainerTest, DirectionalAccess_OutputContainerOnlyDown)
{
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);
    auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::BONE_MEAL != nullptr) {
        ItemStack boneMeal(Items::BONE_MEAL, 1);

        // 任何方向都不能插入
        EXPECT_FALSE(inv->canInsertItem(0, boneMeal, Direction::Up));
        EXPECT_FALSE(inv->canInsertItem(0, boneMeal, Direction::Down));
        EXPECT_FALSE(inv->canInsertItem(0, boneMeal, Direction::North));

        // 从下方可以提取骨粉
        EXPECT_TRUE(inv->canExtractItem(0, boneMeal, Direction::Down));
        // 从其他方向不能提取
        EXPECT_FALSE(inv->canExtractItem(0, boneMeal, Direction::Up));
        EXPECT_FALSE(inv->canExtractItem(0, boneMeal, Direction::North));
        EXPECT_FALSE(inv->canExtractItem(0, boneMeal, Direction::South));
        EXPECT_FALSE(inv->canExtractItem(0, boneMeal, Direction::East));
        EXPECT_FALSE(inv->canExtractItem(0, boneMeal, Direction::West));
    }
}

TEST_F(ComposterContainerTest, DirectionalAccess_EmptyContainerNoAccess)
{
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7);
    auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        ItemStack compostable(Items::APPLE, 1);

        // 任何方向都不能插入或提取
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::Up));
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::Down));
        EXPECT_FALSE(inv->canExtractItem(0, compostable, Direction::Down));
        EXPECT_FALSE(inv->canExtractItem(0, compostable, Direction::Up));
    }
}

// ========== 静态工具方法测试 ==========

TEST_F(ComposterContainerTest, IsCompostable_AppleIsCompostable)
{
    if (Items::APPLE != nullptr) {
        EXPECT_TRUE(ComposterBlock::isCompostable(static_cast<u32>(Items::APPLE->itemId())));
    }
}

TEST_F(ComposterContainerTest, IsCompostable_DiamondIsNotCompostable)
{
    if (Items::DIAMOND != nullptr) {
        EXPECT_FALSE(ComposterBlock::isCompostable(static_cast<u32>(Items::DIAMOND->itemId())));
    }
}

TEST_F(ComposterContainerTest, GetCompostChance_AppleIs65Percent)
{
    if (Items::APPLE != nullptr) {
        EXPECT_FLOAT_EQ(ComposterBlock::getCompostChance(static_cast<u32>(Items::APPLE->itemId())), 0.65f);
    }
}

TEST_F(ComposterContainerTest, GetCompostChance_PumpkinPieIs100Percent)
{
    if (Items::PUMPKIN_PIE != nullptr) {
        EXPECT_FLOAT_EQ(ComposterBlock::getCompostChance(static_cast<u32>(Items::PUMPKIN_PIE->itemId())), 1.0f);
    }
}

TEST_F(ComposterContainerTest, GetCompostChance_NonCompostableIsZero)
{
    if (Items::DIAMOND != nullptr) {
        EXPECT_FLOAT_EQ(ComposterBlock::getCompostChance(static_cast<u32>(Items::DIAMOND->itemId())), 0.0f);
    }
}

// ============================================================================
// Hopper-Composter 集成测试
//
// 验证 InventoryRef 与 ISidedInventoryProvider（ComposterBlock）的集成
// ============================================================================

#include "entity/inventory/InventoryRef.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"

class ComposterHopperIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        Items::initialize();
        CompostableItems::initialize();
    }

    void SetUp() override
    {
        composter_ = std::make_unique<ComposterBlock>(BlockProperties(Material::WOOD).hardness(0.6f).resistance(0.6f));
    }

    std::unique_ptr<ComposterBlock> composter_;
    ComposterContainerTestWorld world_;
};

// ========== InventoryRef 所有权语义测试 ==========

TEST_F(ComposterHopperIntegrationTest, InventoryRef_OwningRefFromComposter)
{
    // 当 ComposterBlock::createInventory() 返回 unique_ptr<ISidedInventory>，
    // InventoryRef 应该拥有所有权
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv, nullptr);

    InventoryRef ref(std::move(inv));
    EXPECT_NE(ref.get(), nullptr);
    EXPECT_TRUE(ref.isOwning());
    EXPECT_FALSE(ref.isEmpty());
    EXPECT_TRUE(static_cast<bool>(ref));

    // ownedSidedInventory 应该返回非空指针（因为拥有 ISidedInventory）
    EXPECT_NE(ref.ownedSidedInventory(), nullptr);
}

TEST_F(ComposterHopperIntegrationTest, InventoryRef_DestructorFreesOwningRef)
{
    // 验证 InventoryRef 析构时正确释放 createInventory 返回的 ISidedInventory
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    {
        std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
        InventoryRef ref(std::move(inv));
        EXPECT_TRUE(ref.isOwning());
        // ref 析构时应该释放 ISidedInventory
    }
    // 如果没有内存泄漏或 double-free，测试通过
}

TEST_F(ComposterHopperIntegrationTest, InventoryRef_MoveTransfersOwnership)
{
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
    ISidedInventory* rawPtr = inv.get();

    InventoryRef ref1(std::move(inv));
    EXPECT_EQ(ref1.ownedSidedInventory(), rawPtr);

    InventoryRef ref2(std::move(ref1));
    EXPECT_EQ(ref1.get(), nullptr);
    EXPECT_TRUE(ref1.isEmpty());
    EXPECT_EQ(ref2.get(), rawPtr);
    EXPECT_TRUE(ref2.isOwning());
    EXPECT_EQ(ref2.ownedSidedInventory(), rawPtr);
}

TEST_F(ComposterHopperIntegrationTest, InventoryRef_ReleaseOwnership)
{
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
    ISidedInventory* rawPtr = inv.get();

    InventoryRef ref(std::move(inv));
    EXPECT_TRUE(ref.isOwning());

    auto released = ref.releaseOwnership();
    EXPECT_EQ(released.get(), rawPtr);
    EXPECT_EQ(ref.get(), nullptr);
    EXPECT_FALSE(ref.isOwning());
    EXPECT_TRUE(ref.isEmpty());

    // released 现在拥有所有权，析构时释放
}

TEST_F(ComposterHopperIntegrationTest, InventoryRef_AccessThroughGet)
{
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));

    InventoryRef ref(std::move(inv));

    // 通过 get() 访问 IInventory 接口
    IInventory* inventory = ref.get();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), 1);

    // 通过 -> 操作符访问
    EXPECT_EQ(ref->getContainerSize(), 1);
}

// ========== ISidedInventory 方向性交互测试（模拟漏斗交互）==========

TEST_F(ComposterHopperIntegrationTest, HopperInsertsIntoComposterFromTop)
{
    // 模拟漏斗从上方（Direction::Up）向堆肥桶输入物品
    // Level 0-6 的堆肥桶应该通过 InputContainer 接受输入
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(5, 10, 15));
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        ItemStack compostable(Items::APPLE, 1);

        // 检查漏斗从上方可以访问的槽位
        auto slots = inv->getSlotsForFace(Direction::Up);
        ASSERT_EQ(slots.size(), 1u);
        EXPECT_EQ(slots[0], 0);

        // 检查可以插入
        EXPECT_TRUE(inv->canInsertItem(0, compostable, Direction::Up));
    }
}

TEST_F(ComposterHopperIntegrationTest, HopperCannotInsertFromSide)
{
    // 模拟漏斗从侧面无法向堆肥桶输入物品
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(5, 10, 15));
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        ItemStack compostable(Items::APPLE, 1);

        // 侧面方向没有可访问的槽位
        EXPECT_TRUE(inv->getSlotsForFace(Direction::North).empty());
        EXPECT_TRUE(inv->getSlotsForFace(Direction::South).empty());
        EXPECT_TRUE(inv->getSlotsForFace(Direction::East).empty());
        EXPECT_TRUE(inv->getSlotsForFace(Direction::West).empty());

        // 侧面方向不能插入
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::North));
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::South));
    }
}

TEST_F(ComposterHopperIntegrationTest, HopperCannotInsertFromBottom)
{
    // 模拟漏斗从下方无法向堆肥桶输入物品（下方是提取方向）
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(5, 10, 15));
    ASSERT_NE(inv, nullptr);

    if (Items::APPLE != nullptr) {
        ItemStack compostable(Items::APPLE, 1);

        // 下方没有可访问的槽位（InputContainer 只允许 Up 方向）
        EXPECT_TRUE(inv->getSlotsForFace(Direction::Down).empty());
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::Down));
    }
}

TEST_F(ComposterHopperIntegrationTest, HopperExtractsBoneMealFromBottom)
{
    // 模拟漏斗从下方（Direction::Down）提取堆肥桶的骨粉
    // Level 8 的堆肥桶应该通过 OutputContainer 允许提取
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);
    std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(5, 10, 15));
    ASSERT_NE(inv, nullptr);

    if (Items::BONE_MEAL != nullptr) {
        ItemStack boneMeal(Items::BONE_MEAL, 1);

        // 检查漏斗从下方可以访问的槽位
        auto slots = inv->getSlotsForFace(Direction::Down);
        ASSERT_EQ(slots.size(), 1u);
        EXPECT_EQ(slots[0], 0);

        // 检查可以从下方提取骨粉
        EXPECT_TRUE(inv->canExtractItem(0, boneMeal, Direction::Down));

        // 不能从上方或其他方向提取
        EXPECT_FALSE(inv->canExtractItem(0, boneMeal, Direction::Up));
        EXPECT_FALSE(inv->canExtractItem(0, boneMeal, Direction::North));
    }
}

TEST_F(ComposterHopperIntegrationTest, HopperCannotExtractFromTop)
{
    // 模拟漏斗从上方无法提取堆肥桶中的物品
    // Level 8 的堆肥桶 OutputContainer 只允许从下方提取
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);
    std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(5, 10, 15));
    ASSERT_NE(inv, nullptr);

    if (Items::BONE_MEAL != nullptr) {
        ItemStack boneMeal(Items::BONE_MEAL, 1);

        // 上方没有可访问的槽位（OutputContainer 只允许 Down 方向）
        EXPECT_TRUE(inv->getSlotsForFace(Direction::Up).empty());
        EXPECT_FALSE(inv->canExtractItem(0, boneMeal, Direction::Up));
    }
}

TEST_F(ComposterHopperIntegrationTest, HopperCannotInteractWithLevel7Composter)
{
    // 模拟漏斗无法与 Level 7（过渡状态）的堆肥桶交互
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7);
    std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(5, 10, 15));
    ASSERT_NE(inv, nullptr);

    // EmptyContainer: 0 槽位，不允许任何方向访问
    EXPECT_EQ(inv->getContainerSize(), 0);
    EXPECT_TRUE(inv->isEmpty());

    // 所有方向的槽位为空
    EXPECT_TRUE(inv->getSlotsForFace(Direction::Up).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::Down).empty());
    EXPECT_TRUE(inv->getSlotsForFace(Direction::North).empty());

    if (Items::APPLE != nullptr) {
        ItemStack compostable(Items::APPLE, 1);
        EXPECT_FALSE(inv->canInsertItem(0, compostable, Direction::Up));
        EXPECT_FALSE(inv->canExtractItem(0, compostable, Direction::Down));
    }
}

TEST_F(ComposterHopperIntegrationTest, ComposterLevelTransitions)
{
    // 测试堆肥桶在不同等级下创建不同类型的容器
    // Level 0-6: InputContainer（可从上方输入）
    // Level 7: EmptyContainer（不允许任何交互）
    // Level 8: OutputContainer（可从下方提取）

    // Level 0: InputContainer
    {
        auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
        auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
        ASSERT_NE(inv, nullptr);
        EXPECT_EQ(inv->getContainerSize(), 1);
        EXPECT_FALSE(inv->getSlotsForFace(Direction::Up).empty());  // 输入方向
        EXPECT_TRUE(inv->getSlotsForFace(Direction::Down).empty()); // 不允许提取
    }

    // Level 3: InputContainer
    {
        auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 3);
        auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
        ASSERT_NE(inv, nullptr);
        EXPECT_EQ(inv->getContainerSize(), 1);
        EXPECT_FALSE(inv->getSlotsForFace(Direction::Up).empty());
        EXPECT_TRUE(inv->getSlotsForFace(Direction::Down).empty());
    }

    // Level 6: InputContainer（最后一个输入等级）
    {
        auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 6);
        auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
        ASSERT_NE(inv, nullptr);
        EXPECT_EQ(inv->getContainerSize(), 1);
        EXPECT_FALSE(inv->getSlotsForFace(Direction::Up).empty());
        EXPECT_TRUE(inv->getSlotsForFace(Direction::Down).empty());
    }

    // Level 7: EmptyContainer
    {
        auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 7);
        auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
        ASSERT_NE(inv, nullptr);
        EXPECT_EQ(inv->getContainerSize(), 0);
        EXPECT_TRUE(inv->getSlotsForFace(Direction::Up).empty());
        EXPECT_TRUE(inv->getSlotsForFace(Direction::Down).empty());
    }

    // Level 8: OutputContainer
    {
        auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 8);
        auto inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
        ASSERT_NE(inv, nullptr);
        EXPECT_EQ(inv->getContainerSize(), 1);
        EXPECT_TRUE(inv->getSlotsForFace(Direction::Up).empty());    // 不允许输入
        EXPECT_FALSE(inv->getSlotsForFace(Direction::Down).empty()); // 输出方向
    }
}

TEST_F(ComposterHopperIntegrationTest, InventoryRef_CreatedPerAccess)
{
    // 验证每次 createInventory() 调用都创建新的容器实例
    // 这是正确的行为：ISidedInventoryProvider 每次都创建新容器
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    auto inv1 = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
    auto inv2 = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
    ASSERT_NE(inv1, nullptr);
    ASSERT_NE(inv2, nullptr);
    // 两个容器是独立的实例
    EXPECT_NE(inv1.get(), inv2.get());
}

TEST_F(ComposterHopperIntegrationTest, InventoryRef_OwningVsNonOwning)
{
    // 漏斗路径：getInventoryAtPosition 对 ISidedInventoryProvider 返回拥有引用
    // 对 BlockEntity/Entity 返回非拥有引用
    // 这里验证两种 InventoryRef 的行为差异

    // 拥有引用（来自 ISidedInventoryProvider）
    auto state = composter_->defaultState().with(BlockStateProperties::LEVEL_0_8(), 0);
    {
        std::unique_ptr<ISidedInventory> inv = composter_->createInventory(state, world_, BlockPos(0, 0, 0));
        InventoryRef owningRef(std::move(inv));
        EXPECT_TRUE(owningRef.isOwning());
        EXPECT_NE(owningRef.ownedSidedInventory(), nullptr);
    }

    // 非拥有引用（模拟 BlockEntity 路径）
    {
        blockentity::SimpleInventory simpleInv(5);
        InventoryRef nonOwningRef(&simpleInv);
        EXPECT_FALSE(nonOwningRef.isOwning());
        EXPECT_EQ(nonOwningRef.ownedSidedInventory(), nullptr);
        EXPECT_NE(nonOwningRef.get(), nullptr);

        // 非拥有引用析构时不应影响原始 inventory
    }
}
