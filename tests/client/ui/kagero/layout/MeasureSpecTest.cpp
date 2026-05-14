/**
 * @file MeasureSpecTest.cpp
 * @brief MeasureSpec 单元测试
 *
 * 测试覆盖率目标：95%+
 */

#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero::layout;

class MeasureSpecTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// 构造函数测试
// ============================================================================

TEST_F(MeasureSpecTest, DefaultConstruction)
{
    MeasureSpec spec;
    EXPECT_EQ(spec.size, 0);
    EXPECT_EQ(spec.mode, MeasureMode::Unspecified);
}

TEST_F(MeasureSpecTest, ParameterizedConstruction)
{
    MeasureSpec spec(100, MeasureMode::Exactly);
    EXPECT_EQ(spec.size, 100);
    EXPECT_EQ(spec.mode, MeasureMode::Exactly);
}

// ============================================================================
// 工厂方法测试
// ============================================================================

TEST_F(MeasureSpecTest, MakeExactly)
{
    MeasureSpec spec = MeasureSpec::MakeExactly(200);
    EXPECT_EQ(spec.size, 200);
    EXPECT_EQ(spec.mode, MeasureMode::Exactly);
    EXPECT_TRUE(spec.isExactly());
    EXPECT_FALSE(spec.isAtMost());
    EXPECT_FALSE(spec.isUnspecified());
}

TEST_F(MeasureSpecTest, MakeExactlyNegativeValue)
{
    // 负值应该被转换为0
    MeasureSpec spec = MeasureSpec::MakeExactly(-10);
    EXPECT_EQ(spec.size, 0);
    EXPECT_EQ(spec.mode, MeasureMode::Exactly);
}

TEST_F(MeasureSpecTest, MakeAtMost)
{
    MeasureSpec spec = MeasureSpec::MakeAtMost(300);
    EXPECT_EQ(spec.size, 300);
    EXPECT_EQ(spec.mode, MeasureMode::AtMost);
    EXPECT_FALSE(spec.isExactly());
    EXPECT_TRUE(spec.isAtMost());
    EXPECT_FALSE(spec.isUnspecified());
}

TEST_F(MeasureSpecTest, MakeAtMostNegativeValue)
{
    MeasureSpec spec = MeasureSpec::MakeAtMost(-5);
    EXPECT_EQ(spec.size, 0);
    EXPECT_EQ(spec.mode, MeasureMode::AtMost);
}

TEST_F(MeasureSpecTest, MakeUnspecified)
{
    MeasureSpec spec = MeasureSpec::MakeUnspecified();
    EXPECT_EQ(spec.size, 0);
    EXPECT_EQ(spec.mode, MeasureMode::Unspecified);
    EXPECT_FALSE(spec.isExactly());
    EXPECT_FALSE(spec.isAtMost());
    EXPECT_TRUE(spec.isUnspecified());
}

// ============================================================================
// 模式检查测试
// ============================================================================

TEST_F(MeasureSpecTest, IsExactly)
{
    EXPECT_TRUE(MeasureSpec::MakeExactly(100).isExactly());
    EXPECT_FALSE(MeasureSpec::MakeAtMost(100).isExactly());
    EXPECT_FALSE(MeasureSpec::MakeUnspecified().isExactly());
}

TEST_F(MeasureSpecTest, IsAtMost)
{
    EXPECT_FALSE(MeasureSpec::MakeExactly(100).isAtMost());
    EXPECT_TRUE(MeasureSpec::MakeAtMost(100).isAtMost());
    EXPECT_FALSE(MeasureSpec::MakeUnspecified().isAtMost());
}

TEST_F(MeasureSpecTest, IsUnspecified)
{
    EXPECT_FALSE(MeasureSpec::MakeExactly(100).isUnspecified());
    EXPECT_FALSE(MeasureSpec::MakeAtMost(100).isUnspecified());
    EXPECT_TRUE(MeasureSpec::MakeUnspecified().isUnspecified());
}

// ============================================================================
// resolve 测试
// ============================================================================

TEST_F(MeasureSpecTest, ResolveExactly)
{
    MeasureSpec spec = MeasureSpec::MakeExactly(100);

    // Exactly模式忽略测量结果，返回规格尺寸
    EXPECT_EQ(spec.resolve(50), 100);
    EXPECT_EQ(spec.resolve(100), 100);
    EXPECT_EQ(spec.resolve(150), 100);
}

TEST_F(MeasureSpecTest, ResolveAtMost)
{
    MeasureSpec spec = MeasureSpec::MakeAtMost(100);

    // AtMost模式返回min(规格尺寸, 测量尺寸)
    EXPECT_EQ(spec.resolve(50), 50);   // 测量值小于规格
    EXPECT_EQ(spec.resolve(100), 100); // 测量值等于规格
    EXPECT_EQ(spec.resolve(150), 100); // 测量值大于规格，返回规格
}

TEST_F(MeasureSpecTest, ResolveUnspecified)
{
    MeasureSpec spec = MeasureSpec::MakeUnspecified();

    // Unspecified模式返回测量值
    EXPECT_EQ(spec.resolve(50), 50);
    EXPECT_EQ(spec.resolve(100), 100);
    EXPECT_EQ(spec.resolve(150), 150);
}

// ============================================================================
// adjust 测试
// ============================================================================

TEST_F(MeasureSpecTest, AdjustExactly)
{
    MeasureSpec spec = MeasureSpec::MakeExactly(100);

    // Exactly模式返回规格尺寸
    EXPECT_EQ(spec.adjust(50), 100);
    EXPECT_EQ(spec.adjust(100), 100);
    EXPECT_EQ(spec.adjust(150), 100);
}

TEST_F(MeasureSpecTest, AdjustAtMost)
{
    MeasureSpec spec = MeasureSpec::MakeAtMost(100);

    EXPECT_EQ(spec.adjust(50), 50);
    EXPECT_EQ(spec.adjust(100), 100);
    EXPECT_EQ(spec.adjust(150), 100);
    EXPECT_EQ(spec.adjust(-10), 0); // 负值被转换为0
}

TEST_F(MeasureSpecTest, AdjustUnspecified)
{
    MeasureSpec spec = MeasureSpec::MakeUnspecified();

    EXPECT_EQ(spec.adjust(50), 50);
    EXPECT_EQ(spec.adjust(100), 100);
    EXPECT_EQ(spec.adjust(150), 150);
    EXPECT_EQ(spec.adjust(-10), 0); // 负值被转换为0
}

// ============================================================================
// 比较操作符测试
// ============================================================================

TEST_F(MeasureSpecTest, Equality)
{
    MeasureSpec spec1(100, MeasureMode::Exactly);
    MeasureSpec spec2(100, MeasureMode::Exactly);
    MeasureSpec spec3(200, MeasureMode::Exactly);
    MeasureSpec spec4(100, MeasureMode::AtMost);

    EXPECT_TRUE(spec1 == spec2);
    EXPECT_FALSE(spec1 == spec3);
    EXPECT_FALSE(spec1 == spec4);
}

TEST_F(MeasureSpecTest, Inequality)
{
    MeasureSpec spec1(100, MeasureMode::Exactly);
    MeasureSpec spec2(100, MeasureMode::Exactly);
    MeasureSpec spec3(200, MeasureMode::Exactly);

    EXPECT_FALSE(spec1 != spec2);
    EXPECT_TRUE(spec1 != spec3);
}

// ============================================================================
// Size 结构测试
// ============================================================================

class SizeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SizeTest, DefaultConstruction)
{
    Size size;
    EXPECT_EQ(size.width, 0);
    EXPECT_EQ(size.height, 0);
}

TEST_F(SizeTest, ParameterizedConstruction)
{
    Size size(100, 200);
    EXPECT_EQ(size.width, 100);
    EXPECT_EQ(size.height, 200);
}

TEST_F(SizeTest, IsValid)
{
    EXPECT_TRUE(Size(100, 200).isValid());
    EXPECT_TRUE(Size(0, 0).isValid());
    EXPECT_FALSE(Size(-1, 100).isValid());
    EXPECT_FALSE(Size(100, -1).isValid());
    EXPECT_FALSE(Size(-10, -20).isValid());
}

TEST_F(SizeTest, Unlimited)
{
    Size unlimited = Size::unlimited();
    EXPECT_TRUE(unlimited.isValid());
    EXPECT_GT(unlimited.width, 1000000000);
    EXPECT_GT(unlimited.height, 1000000000);
}

TEST_F(SizeTest, Equality)
{
    Size size1(100, 200);
    Size size2(100, 200);
    Size size3(200, 100);

    EXPECT_TRUE(size1 == size2);
    EXPECT_FALSE(size1 == size3);
}

TEST_F(SizeTest, Inequality)
{
    Size size1(100, 200);
    Size size2(100, 200);
    Size size3(200, 100);

    EXPECT_FALSE(size1 != size2);
    EXPECT_TRUE(size1 != size3);
}
