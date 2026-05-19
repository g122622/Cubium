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

#include "client/dimension/ClientDimensionManager.hpp"
#include <gtest/gtest.h>

using namespace mc;

namespace {

/**
 * @brief 测试默认构造
 */
TEST(ClientDimensionManagerTest, DefaultConstruction)
{
    ClientDimensionManager manager;

    // 默认状态
    EXPECT_EQ(manager.currentDimension(), 0);
    EXPECT_EQ(manager.transitionState(), ClientDimensionManager::TransitionState::None);
    EXPECT_FALSE(manager.isChangingDimension());
    EXPECT_FALSE(manager.needsRenderReset());
}

/**
 * @brief 测试使用 ID 列表初始化
 */
TEST(ClientDimensionManagerTest, InitializeWithIds)
{
    ClientDimensionManager manager;

    std::vector<DimensionId> ids = {0, -1, 1}; // Overworld, Nether, End
    manager.initialize(ids);

    // 检查可用维度
    const auto& availableIds = manager.availableDimensions();
    EXPECT_EQ(availableIds.size(), 3);
    EXPECT_TRUE(manager.isDimensionAvailable(0));
    EXPECT_TRUE(manager.isDimensionAvailable(-1));
    EXPECT_TRUE(manager.isDimensionAvailable(1));
    EXPECT_FALSE(manager.isDimensionAvailable(2)); // 不存在的维度

    // 检查维度信息被正确推断
    const auto* overworldInfo = manager.getDimensionInfo(0);
    ASSERT_NE(overworldInfo, nullptr);
    EXPECT_EQ(overworldInfo->id, 0);
    EXPECT_EQ(overworldInfo->name, "minecraft:overworld");
    EXPECT_TRUE(overworldInfo->hasSkyLight);
    EXPECT_FALSE(overworldInfo->hasCeiling);

    const auto* netherInfo = manager.getDimensionInfo(-1);
    ASSERT_NE(netherInfo, nullptr);
    EXPECT_EQ(netherInfo->id, -1);
    EXPECT_EQ(netherInfo->name, "minecraft:the_nether");
    EXPECT_FALSE(netherInfo->hasSkyLight);
    EXPECT_TRUE(netherInfo->hasCeiling);

    const auto* endInfo = manager.getDimensionInfo(1);
    ASSERT_NE(endInfo, nullptr);
    EXPECT_EQ(endInfo->id, 1);
    EXPECT_EQ(endInfo->name, "minecraft:the_end");
    EXPECT_FALSE(endInfo->hasSkyLight);
    EXPECT_FALSE(endInfo->hasCeiling);
}

/**
 * @brief 测试使用完整维度信息初始化
 */
TEST(ClientDimensionManagerTest, InitializeWithFullInfo)
{
    ClientDimensionManager manager;

    std::vector<ClientDimensionInfo> infos = {{0, "minecraft:overworld", true, false, 0.0f},
        {-1, "minecraft:the_nether", false, true, 0.1f},
        {1, "minecraft:the_end", false, false, 0.0f}};

    manager.initialize(infos);

    // 检查维度信息
    const auto& availableInfos = manager.availableDimensionInfos();
    EXPECT_EQ(availableInfos.size(), 3);

    const auto* overworldInfo = manager.getDimensionInfo(0);
    ASSERT_NE(overworldInfo, nullptr);
    EXPECT_EQ(overworldInfo->name, "minecraft:overworld");
    EXPECT_FLOAT_EQ(overworldInfo->ambientLight, 0.0f);

    const auto* netherInfo = manager.getDimensionInfo(-1);
    ASSERT_NE(netherInfo, nullptr);
    EXPECT_EQ(netherInfo->name, "minecraft:the_nether");
    EXPECT_FLOAT_EQ(netherInfo->ambientLight, 0.1f);
}

/**
 * @brief 测试空初始化自动添加主世界
 */
TEST(ClientDimensionManagerTest, EmptyInitializeAddsOverworld)
{
    ClientDimensionManager manager;

    std::vector<DimensionId> emptyIds;
    manager.initialize(emptyIds);

    // 应该自动添加主世界
    const auto& availableIds = manager.availableDimensions();
    EXPECT_EQ(availableIds.size(), 1);
    EXPECT_TRUE(manager.isDimensionAvailable(0));
    EXPECT_EQ(manager.currentDimension(), 0);
}

/**
 * @brief 测试重置
 */
TEST(ClientDimensionManagerTest, Reset)
{
    ClientDimensionManager manager;

    std::vector<ClientDimensionInfo> infos = {
        {0, "minecraft:overworld", true, false, 0.0f}, {-1, "minecraft:the_nether", false, true, 0.1f}};
    manager.initialize(infos);
    manager.setCurrentDimension(-1);
    manager.beginDimensionChange(1, Vector3d(100.0, 64.0, 200.0));

    manager.reset();

    // 重置后应恢复默认状态
    EXPECT_EQ(manager.currentDimension(), 0);
    EXPECT_EQ(manager.transitionState(), ClientDimensionManager::TransitionState::None);
    EXPECT_TRUE(manager.availableDimensions().empty());
    EXPECT_FALSE(manager.needsRenderReset());
}

/**
 * @brief 测试维度切换流程
 */
TEST(ClientDimensionManagerTest, DimensionChange)
{
    ClientDimensionManager manager;

    std::vector<DimensionId> ids = {0, -1, 1};
    manager.initialize(ids);

    // 开始维度切换
    manager.beginDimensionChange(-1, Vector3d(100.0, 64.0, 200.0));

    EXPECT_EQ(manager.transitionState(), ClientDimensionManager::TransitionState::Leaving);
    EXPECT_TRUE(manager.isChangingDimension());
    EXPECT_EQ(manager.targetDimension(), -1);
    EXPECT_DOUBLE_EQ(manager.targetPosition().x, 100.0);
    EXPECT_TRUE(manager.needsRenderReset());

    // 完成切换
    manager.completeDimensionChange();

    EXPECT_EQ(manager.currentDimension(), -1);
    EXPECT_EQ(manager.transitionState(), ClientDimensionManager::TransitionState::None);
    EXPECT_FALSE(manager.isChangingDimension());
    EXPECT_EQ(manager.targetDimension(), 0); // 重置为默认值
}

/**
 * @brief 测试取消维度切换
 */
TEST(ClientDimensionManagerTest, CancelDimensionChange)
{
    ClientDimensionManager manager;

    std::vector<DimensionId> ids = {0, -1};
    manager.initialize(ids);
    manager.beginDimensionChange(-1, Vector3d(100.0, 64.0, 200.0));

    manager.cancelDimensionChange();

    EXPECT_EQ(manager.currentDimension(), 0); // 保持原维度
    EXPECT_EQ(manager.transitionState(), ClientDimensionManager::TransitionState::None);
    EXPECT_FALSE(manager.isChangingDimension());
    EXPECT_EQ(manager.targetDimension(), 0);
    EXPECT_DOUBLE_EQ(manager.targetPosition().x, 0.0); // 重置为默认值
}

/**
 * @brief 测试设置当前维度
 */
TEST(ClientDimensionManagerTest, SetCurrentDimension)
{
    ClientDimensionManager manager;

    std::vector<DimensionId> ids = {0, -1, 1};
    manager.initialize(ids);
    manager.markRenderReset(); // 清除之前的标记

    // 设置相同维度不应触发渲染重置
    manager.setCurrentDimension(0);
    EXPECT_FALSE(manager.needsRenderReset());

    // 设置不同维度应触发渲染重置
    manager.setCurrentDimension(-1);
    EXPECT_TRUE(manager.needsRenderReset());
    EXPECT_EQ(manager.currentDimension(), -1);

    // 再次标记清除
    manager.markRenderReset();

    // 再次设置相同维度不应触发渲染重置
    manager.setCurrentDimension(-1);
    EXPECT_FALSE(manager.needsRenderReset());
}

/**
 * @brief 测试获取维度类型
 */
TEST(ClientDimensionManagerTest, GetDimensionType)
{
    ClientDimensionManager manager;

    std::vector<DimensionId> ids = {0, -1, 1};
    manager.initialize(ids);

    // 主世界
    const DimensionType* overworldType = manager.getDimensionType(0);
    ASSERT_NE(overworldType, nullptr);
    EXPECT_EQ(overworldType->id(), 0);
    EXPECT_TRUE(overworldType->hasSkyLight());
    EXPECT_FALSE(overworldType->hasCeiling());
    EXPECT_FLOAT_EQ(overworldType->coordinateScale(), 1.0f);

    // 下界
    const DimensionType* netherType = manager.getDimensionType(-1);
    ASSERT_NE(netherType, nullptr);
    EXPECT_EQ(netherType->id(), -1);
    EXPECT_FALSE(netherType->hasSkyLight());
    EXPECT_TRUE(netherType->hasCeiling());
    EXPECT_FLOAT_EQ(netherType->coordinateScale(), 8.0f);

    // 末地
    const DimensionType* endType = manager.getDimensionType(1);
    ASSERT_NE(endType, nullptr);
    EXPECT_EQ(endType->id(), 1);
    EXPECT_FALSE(endType->hasSkyLight());
    EXPECT_FALSE(endType->hasCeiling());

    // 不存在的维度
    const DimensionType* unknownType = manager.getDimensionType(999);
    EXPECT_EQ(unknownType, nullptr);
}

/**
 * @brief 测试获取当前维度类型
 */
TEST(ClientDimensionManagerTest, CurrentDimensionType)
{
    ClientDimensionManager manager;

    std::vector<DimensionId> ids = {0, -1, 1};
    manager.initialize(ids);

    // 默认在主世界
    const DimensionType* currentType = manager.currentDimensionType();
    ASSERT_NE(currentType, nullptr);
    EXPECT_EQ(currentType->id(), 0);

    // 切换到下界
    manager.setCurrentDimension(-1);
    currentType = manager.currentDimensionType();
    ASSERT_NE(currentType, nullptr);
    EXPECT_EQ(currentType->id(), -1);
}

/**
 * @brief 测试标记渲染重置
 */
TEST(ClientDimensionManagerTest, RenderReset)
{
    ClientDimensionManager manager;

    EXPECT_FALSE(manager.needsRenderReset());

    // beginDimensionChange 应设置标记
    manager.beginDimensionChange(-1, Vector3d(0.0, 0.0, 0.0));
    EXPECT_TRUE(manager.needsRenderReset());

    // markRenderReset 应清除标记
    manager.markRenderReset();
    EXPECT_FALSE(manager.needsRenderReset());
}

/**
 * @brief 测试获取不存在维度的信息
 */
TEST(ClientDimensionManagerTest, GetNonExistentDimensionInfo)
{
    ClientDimensionManager manager;

    std::vector<DimensionId> ids = {0}; // 只有主世界
    manager.initialize(ids);

    const ClientDimensionInfo* info = manager.getDimensionInfo(-1);
    EXPECT_EQ(info, nullptr);

    info = manager.getDimensionInfo(999);
    EXPECT_EQ(info, nullptr);
}

} // namespace
