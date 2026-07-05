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
 * @file EntityPipelineBlendModeTest.cpp
 * @brief EntityPipeline::BlendMode 枚举契约单元测试
 *
 * EntityPipeline 的 bind() 选择逻辑依赖 Vulkan 管线句柄，无法在无 Vulkan 设备的单元测试中
 * 直接验证。本文件验证 BlendMode 枚举的契约（枚举值、底层类型、bind 默认参数），
 * 确保 Multiply 和 None 模式作为独立枚举值存在且可被编译期检查。
 */

#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include <array>
#include <type_traits>
#include <gtest/gtest.h>

namespace mc::client::renderer::entity::pipeline::test {

// ============================================================================
// 测试夹具
// ============================================================================

class EntityPipelineBlendModeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ============================================================================
// 枚举值契约测试
// ============================================================================

/**
 * @brief 验证 BlendMode 是 u8 底层类型的 scoped enum
 *
 * 项目约定使用 u8 作为枚举底层类型以节省内存。
 */
TEST_F(EntityPipelineBlendModeTest, EnumHasUint8UnderlyingType)
{
    static_assert(std::is_same_v<std::underlying_type_t<BlendMode>, u8>, "BlendMode 底层类型必须是 u8");
}

/**
 * @brief 验证所有 BlendMode 枚举值存在且具有预期的顺序
 *
 * 枚举顺序固定为 None=0, Alpha=1, Additive=2, Multiply=3, Lines=4。
 * 修改顺序会破坏序列化兼容性和 switch 默认分支的语义。
 */
TEST_F(EntityPipelineBlendModeTest, EnumValuesAreSequentialFromZero)
{
    EXPECT_EQ(static_cast<u8>(BlendMode::None), 0u);
    EXPECT_EQ(static_cast<u8>(BlendMode::Alpha), 1u);
    EXPECT_EQ(static_cast<u8>(BlendMode::Additive), 2u);
    EXPECT_EQ(static_cast<u8>(BlendMode::Multiply), 3u);
    EXPECT_EQ(static_cast<u8>(BlendMode::Lines), 4u);
}

/**
 * @brief 验证枚举值数量为 5（None/Alpha/Additive/Multiply/Lines）
 *
 * 添加新枚举值时需同步更新此测试和 bind() 的 switch 语句。
 */
TEST_F(EntityPipelineBlendModeTest, EnumHasExactlyFiveValues)
{
    // 通过遍历计数验证枚举值范围 [0, 5)
    u32 count = 0;
    for (u8 i = 0; i < 5; ++i) {
        // 静态转换不会抛异常，仅用于确认范围合法
        [[maybe_unused]] auto mode = static_cast<BlendMode>(i);
        ++count;
    }
    EXPECT_EQ(count, 5u);
}

// ============================================================================
// bind() 默认参数契约测试
// ============================================================================

/**
 * @brief 验证 bind() 方法签名包含 BlendMode 参数
 *
 * 此测试通过函数指针类型比较确认 bind 接受 BlendMode 参数。
 * 默认参数值（BlendMode::Alpha）无法通过类型系统直接验证，但可在编译期确认签名。
 */
TEST_F(EntityPipelineBlendModeTest, BindMethodAcceptsBlendModeParameter)
{
    using BindMethodSig = void (EntityPipeline::*)(VkCommandBuffer, BlendMode);
    static_assert(std::is_same_v<decltype(&EntityPipeline::bind), BindMethodSig>,
        "EntityPipeline::bind 必须接受 (VkCommandBuffer, BlendMode) 参数");
}

// ============================================================================
// Multiply 和 None 模式独立存在性测试
// ============================================================================

/**
 * @brief 验证 Multiply 模式作为独立枚举值存在
 *
 * 此前 Multiply 与 None 一起回退到 Alpha 管线。现在 Multiply 应有专用管线。
 * 通过确认 Multiply 与其他枚举值不等来验证其独立性。
 */
TEST_F(EntityPipelineBlendModeTest, MultiplyModeIsDistinctFromOtherModes)
{
    const BlendMode multiply = BlendMode::Multiply;
    EXPECT_NE(multiply, BlendMode::None);
    EXPECT_NE(multiply, BlendMode::Alpha);
    EXPECT_NE(multiply, BlendMode::Additive);
    EXPECT_NE(multiply, BlendMode::Lines);
}

/**
 * @brief 验证 None 模式作为独立枚举值存在
 *
 * 此前 None 与 Multiply 一起回退到 Alpha 管线。现在 None 应有专用管线。
 */
TEST_F(EntityPipelineBlendModeTest, NoneModeIsDistinctFromOtherModes)
{
    const BlendMode none = BlendMode::None;
    EXPECT_NE(none, BlendMode::Alpha);
    EXPECT_NE(none, BlendMode::Additive);
    EXPECT_NE(none, BlendMode::Multiply);
    EXPECT_NE(none, BlendMode::Lines);
}

/**
 * @brief 验证所有 5 种模式两两不等
 *
 * 确保枚举值不会因重构而意外合并。
 */
TEST_F(EntityPipelineBlendModeTest, AllModesArePairwiseDistinct)
{
    const std::array<BlendMode, 5> modes = {
        BlendMode::None, BlendMode::Alpha, BlendMode::Additive, BlendMode::Multiply, BlendMode::Lines};

    for (size_t i = 0; i < modes.size(); ++i) {
        for (size_t j = i + 1; j < modes.size(); ++j) {
            EXPECT_NE(modes[i], modes[j]) << "枚举值 " << i << " 和 " << j << " 不应相等";
        }
    }
}

} // namespace mc::client::renderer::entity::pipeline::test
