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

/**
 * @file HeldBlockLayerTest.cpp
 * @brief HeldBlockLayer 核心逻辑单元测试
 *
 * 测试末影人手持方块渲染层的核心逻辑，包括：
 * - EndermanEntity 的 isHoldingBlock() 方法
 * - EndermanEntity 的 getHeldBlockState() 方法
 * - 编译时类型检查逻辑 (std::is_base_of_v)
 * - 非 EndermanEntity 类型的默认行为
 *
 * 注意：由于 HeldBlockLayer 是模板类且依赖 Vulkan 渲染管线，
 * 本测试专注于 EndermanEntity 提供的核心接口，不测试完整渲染流程。
 */

#include <type_traits>
#include <gtest/gtest.h>

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"

using namespace mc;

namespace mc::renderer::layer::test {

/**
 * @brief EndermanEntity 手持方块功能测试
 *
 * 这些测试验证 EndermanEntity 提供给 HeldBlockLayer 的核心接口
 */
class EndermanBlockHoldingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化测试
    }

    void TearDown() override
    {
        // 清理测试
    }
};

// ============================================================================
// 编译时类型检查测试
// ============================================================================

/**
 * @brief 测试 EndermanEntity 是 LivingEntity 的子类
 *
 * 验证继承关系，确保 HeldBlockLayer 模板类型检查能够正确工作
 */
TEST_F(EndermanBlockHoldingTest, EndermanEntityIsLivingEntity)
{
    EXPECT_TRUE((std::is_base_of_v<LivingEntity, EndermanEntity>)) << "EndermanEntity should derive from LivingEntity";
}

/**
 * @brief 测试 std::is_base_of_v 类型检查的正确性
 *
 * 这是 HeldBlockLayer 中 if constexpr 检查的核心机制
 */
TEST_F(EndermanBlockHoldingTest, TypeCheckEndermanEntity)
{
    // EndermanEntity 是自身的基类
    EXPECT_TRUE((std::is_base_of_v<EndermanEntity, EndermanEntity>)) << "EndermanEntity is base of itself";

    EXPECT_TRUE((std::is_base_of_v<EndermanEntity, const EndermanEntity>))
        << "EndermanEntity is base of const EndermanEntity";

    // LivingEntity 不是 EndermanEntity 的子类
    EXPECT_FALSE((std::is_base_of_v<EndermanEntity, LivingEntity>))
        << "LivingEntity is NOT a derived class of EndermanEntity";
}

/**
 * @brief 测试对非 EndermanEntity 类型的类型检查
 */
TEST_F(EndermanBlockHoldingTest, TypeCheckNonEndermanEntity)
{
    // LivingEntity 不满足 is_base_of<EndermanEntity, LivingEntity>
    EXPECT_FALSE((std::is_base_of_v<EndermanEntity, LivingEntity>))
        << "LivingEntity should NOT satisfy is_base_of<EndermanEntity, LivingEntity>";

    // Entity 也不满足
    EXPECT_FALSE((std::is_base_of_v<EndermanEntity, Entity>))
        << "Entity should NOT satisfy is_base_of<EndermanEntity, Entity>";
}

// ============================================================================
// EndermanEntity 方块持有功能测试
// ============================================================================

/**
 * @brief 测试 EndermanEntity 默认不持有方块
 *
 * 新创建的 EndermanEntity 实例应该不持有任何方块
 */
TEST_F(EndermanBlockHoldingTest, DefaultNoBlock)
{
    EndermanEntity enderman(LegacyEntityType::Enderman, EntityId(1));

    EXPECT_FALSE(enderman.isHoldingBlock()) << "New EndermanEntity should not be holding a block by default";

    EXPECT_EQ(enderman.getHeldBlockState(), nullptr)
        << "New EndermanEntity should have null held block state by default";
}

/**
 * @brief 测试 EndermanEntity 设置持有方块
 *
 * 验证 setHeldBlockState 正确更新持有状态
 */
TEST_F(EndermanBlockHoldingTest, SetHeldBlock)
{
    EndermanEntity enderman(LegacyEntityType::Enderman, EntityId(1));

    // 使用一个假的 BlockState 指针进行测试
    // 实际渲染时会从 BlockRegistry 获取真实的 BlockState
    const BlockState* fakeBlockState = reinterpret_cast<const BlockState*>(0x1000);

    enderman.setHeldBlockState(fakeBlockState);

    EXPECT_TRUE(enderman.isHoldingBlock()) << "EndermanEntity should be holding block after setHeldBlockState";

    EXPECT_EQ(enderman.getHeldBlockState(), fakeBlockState) << "getHeldBlockState should return the set block state";
}

/**
 * @brief 测试 EndermanEntity 设置空指针清除方块
 *
 * 验证 setHeldBlockState(nullptr) 正确清除持有状态
 */
TEST_F(EndermanBlockHoldingTest, ClearHeldBlock)
{
    EndermanEntity enderman(LegacyEntityType::Enderman, EntityId(1));

    // 先设置一个方块
    const BlockState* fakeBlockState = reinterpret_cast<const BlockState*>(0x1000);
    enderman.setHeldBlockState(fakeBlockState);
    EXPECT_TRUE(enderman.isHoldingBlock());

    // 清除方块
    enderman.setHeldBlockState(nullptr);

    EXPECT_FALSE(enderman.isHoldingBlock())
        << "EndermanEntity should not be holding block after setHeldBlockState(nullptr)";

    EXPECT_EQ(enderman.getHeldBlockState(), nullptr) << "getHeldBlockState should return nullptr after clearing";
}

/**
 * @brief 测试 EndermanEntity 多次设置方块状态
 *
 * 验证重复设置方块状态的正确性
 */
TEST_F(EndermanBlockHoldingTest, MultipleSetHeldBlock)
{
    EndermanEntity enderman(LegacyEntityType::Enderman, EntityId(1));

    const BlockState* state1 = reinterpret_cast<const BlockState*>(0x1000);
    const BlockState* state2 = reinterpret_cast<const BlockState*>(0x2000);

    // 第一次设置
    enderman.setHeldBlockState(state1);
    EXPECT_EQ(enderman.getHeldBlockState(), state1);
    EXPECT_TRUE(enderman.isHoldingBlock());

    // 第二次设置（不同状态）
    enderman.setHeldBlockState(state2);
    EXPECT_EQ(enderman.getHeldBlockState(), state2);
    EXPECT_TRUE(enderman.isHoldingBlock());

    // 清除
    enderman.setHeldBlockState(nullptr);
    EXPECT_EQ(enderman.getHeldBlockState(), nullptr);
    EXPECT_FALSE(enderman.isHoldingBlock());

    // 再次设置
    enderman.setHeldBlockState(state1);
    EXPECT_EQ(enderman.getHeldBlockState(), state1);
    EXPECT_TRUE(enderman.isHoldingBlock());
}

// ============================================================================
// shouldRender 逻辑模拟测试
// ============================================================================

/**
 * @brief 测试 shouldRender 逻辑：不持有方块时不渲染
 *
 * 模拟 HeldBlockLayer::shouldRender 的逻辑
 */
TEST_F(EndermanBlockHoldingTest, ShouldRenderLogicNoBlock)
{
    EndermanEntity enderman(LegacyEntityType::Enderman, EntityId(1));

    // 模拟 HeldBlockLayer::shouldRender 的逻辑
    // if constexpr (std::is_base_of_v<EndermanEntity, TEntity>) {
    //     return entity.isHoldingBlock();
    // }
    // return false;

    bool shouldRender = false;
    if constexpr (std::is_base_of_v<EndermanEntity, EndermanEntity>) {
        shouldRender = enderman.isHoldingBlock();
    }

    EXPECT_FALSE(shouldRender) << "shouldRender should be false when EndermanEntity is not holding a block";
}

/**
 * @brief 测试 shouldRender 逻辑：持有方块时渲染
 *
 * 模拟 HeldBlockLayer::shouldRender 的逻辑
 */
TEST_F(EndermanBlockHoldingTest, ShouldRenderLogicWithBlock)
{
    EndermanEntity enderman(LegacyEntityType::Enderman, EntityId(1));

    // 设置持有方块
    const BlockState* fakeBlockState = reinterpret_cast<const BlockState*>(0x1000);
    enderman.setHeldBlockState(fakeBlockState);

    // 模拟 HeldBlockLayer::shouldRender 的逻辑
    bool shouldRender = false;
    if constexpr (std::is_base_of_v<EndermanEntity, EndermanEntity>) {
        shouldRender = enderman.isHoldingBlock();
    }

    EXPECT_TRUE(shouldRender) << "shouldRender should be true when EndermanEntity is holding a block";
}

/**
 * @brief 测试 shouldRender 对非 EndermanEntity 类型的默认行为
 *
 * 对于 LivingEntity 类型，shouldRender 应该返回 false
 */
TEST_F(EndermanBlockHoldingTest, ShouldRenderLogicNonEndermanEntity)
{
    // 模拟 HeldBlockLayer::shouldRender 对非 EndermanEntity 类型的逻辑
    // if constexpr (std::is_base_of_v<EndermanEntity, TEntity>) {
    //     return entity.isHoldingBlock();
    // }
    // return false;

    bool shouldRender = false;
    if constexpr (std::is_base_of_v<EndermanEntity, LivingEntity>) {
        // 这个分支不应该被执行，因为 LivingEntity 不是 EndermanEntity 的子类
        shouldRender = true; // 这行代码不会被执行
    }
    // 默认返回 false

    EXPECT_FALSE(shouldRender) << "shouldRender should be false for non-EndermanEntity types (like LivingEntity)";
}

// ============================================================================
// getHeldBlock 逻辑模拟测试
// ============================================================================

/**
 * @brief 测试 getHeldBlock 逻辑：不持有方块时返回 nullptr
 *
 * 模拟 HeldBlockLayer::getHeldBlock 的逻辑
 */
TEST_F(EndermanBlockHoldingTest, GetHeldBlockLogicNoBlock)
{
    EndermanEntity enderman(LegacyEntityType::Enderman, EntityId(1));

    // 模拟 HeldBlockLayer::getHeldBlock 的逻辑
    // if constexpr (std::is_base_of_v<EndermanEntity, TEntity>) {
    //     return entity.getHeldBlockState();
    // }
    // return nullptr;

    const BlockState* heldBlock = nullptr;
    if constexpr (std::is_base_of_v<EndermanEntity, EndermanEntity>) {
        heldBlock = enderman.getHeldBlockState();
    }

    EXPECT_EQ(heldBlock, nullptr) << "getHeldBlock should return nullptr when EndermanEntity is not holding a block";
}

/**
 * @brief 测试 getHeldBlock 逻辑：持有方块时返回正确的 BlockState
 *
 * 模拟 HeldBlockLayer::getHeldBlock 的逻辑
 */
TEST_F(EndermanBlockHoldingTest, GetHeldBlockLogicWithBlock)
{
    EndermanEntity enderman(LegacyEntityType::Enderman, EntityId(1));

    // 设置持有方块
    const BlockState* fakeBlockState = reinterpret_cast<const BlockState*>(0x1000);
    enderman.setHeldBlockState(fakeBlockState);

    // 模拟 HeldBlockLayer::getHeldBlock 的逻辑
    const BlockState* heldBlock = nullptr;
    if constexpr (std::is_base_of_v<EndermanEntity, EndermanEntity>) {
        heldBlock = enderman.getHeldBlockState();
    }

    EXPECT_EQ(heldBlock, fakeBlockState) << "getHeldBlock should return the correct BlockState";
}

/**
 * @brief 测试 getHeldBlock 对非 EndermanEntity 类型的默认行为
 *
 * 对于 LivingEntity 类型，getHeldBlock 应该返回 nullptr
 */
TEST_F(EndermanBlockHoldingTest, GetHeldBlockLogicNonEndermanEntity)
{
    // 模拟 HeldBlockLayer::getHeldBlock 对非 EndermanEntity 类型的逻辑
    // if constexpr (std::is_base_of_v<EndermanEntity, TEntity>) {
    //     return entity.getHeldBlockState();
    // }
    // return nullptr;

    const BlockState* heldBlock = nullptr;
    if constexpr (std::is_base_of_v<EndermanEntity, LivingEntity>) {
        // 这个分支不应该被执行，因为 LivingEntity 不是 EndermanEntity 的子类
        // heldBlock = entity.getHeldBlockState(); // 无法调用，LivingEntity 没有这个方法
    }
    // 默认返回 nullptr

    EXPECT_EQ(heldBlock, nullptr) << "getHeldBlock should return nullptr for non-EndermanEntity types";
}

// ============================================================================
// EndermanEntity 其他状态测试
// ============================================================================

/**
 * @brief 测试 EndermanEntity 的 screaming 状态
 *
 * 验证 EndermanEntity 的 screaming 状态可以正确设置和获取
 * 这个状态被 EndermanRenderer 使用来设置模型动画
 */
TEST_F(EndermanBlockHoldingTest, ScreamingState)
{
    EndermanEntity enderman(LegacyEntityType::Enderman, EntityId(1));

    // 默认不尖叫
    EXPECT_FALSE(enderman.isScreaming()) << "EndermanEntity should not be screaming by default";

    // 设置尖叫状态
    enderman.setScreaming(true);
    EXPECT_TRUE(enderman.isScreaming()) << "EndermanEntity should be screaming after setScreaming(true)";

    // 取消尖叫
    enderman.setScreaming(false);
    EXPECT_FALSE(enderman.isScreaming()) << "EndermanEntity should not be screaming after setScreaming(false)";
}

/**
 * @brief 测试 EndermanEntity 的 angry 状态
 *
 * 验证 EndermanEntity 的 angry 状态可以正确设置和获取
 */
TEST_F(EndermanBlockHoldingTest, AngryState)
{
    EndermanEntity enderman(LegacyEntityType::Enderman, EntityId(1));

    // 默认不愤怒
    EXPECT_FALSE(enderman.isAngry()) << "EndermanEntity should not be angry by default";

    // 设置愤怒状态
    enderman.setAngry(true);
    EXPECT_TRUE(enderman.isAngry()) << "EndermanEntity should be angry after setAngry(true)";

    // 取消愤怒
    enderman.setAngry(false);
    EXPECT_FALSE(enderman.isAngry()) << "EndermanEntity should not be angry after setAngry(false)";
}

} // namespace mc::renderer::layer::test
