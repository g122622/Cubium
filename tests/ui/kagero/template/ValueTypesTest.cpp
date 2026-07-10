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

// Value 类原生 i64/u32/f64 支持测试
// 验证内部 i64/f64 宽存储下，各访问器的精度保持与窄化行为

#include <gtest/gtest.h>

#include <any>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"

using namespace mc::client::ui::kagero::tpl::binder;
using mc::f32;
using mc::f64;
using mc::i32;
using mc::i64;
using mc::u32;
using mc::u64;

// ==================== i64 原生支持测试 ====================

class ValueI64Test : public ::testing::Test {
protected:
    void SetUp() override {}
};

// i64 构造后类型为 Integer
TEST_F(ValueI64Test, ConstructorMarksAsInteger)
{
    Value v(static_cast<i64>(0));
    EXPECT_TRUE(v.isInteger());
    EXPECT_FALSE(v.isNull());
}

// i64 大值（超出 i32 范围）通过 asI64 原生读取，无截断
TEST_F(ValueI64Test, LargeI64PreservedThroughAsI64)
{
    constexpr i64 large = static_cast<i64>(std::numeric_limits<i32>::max()) + 1LL; // 2147483648
    Value v(large);
    EXPECT_EQ(v.asI64(), large);
}

// i64 超出 i32 范围时，asInteger 会窄化截断（行为契约，非精度丢失）
TEST_F(ValueI64Test, LargeI64NarrowedByAsInteger)
{
    constexpr i64 large = 5000000000LL; // 50 亿，远超 i32 范围
    Value v(large);
    // asI64 保持原值，asInteger 窄化后必然不等于原值
    EXPECT_EQ(v.asI64(), large);
    EXPECT_NE(static_cast<i64>(v.asInteger()), large);
}

// i64 负数边界值保持
TEST_F(ValueI64Test, NegativeBoundaryPreserved)
{
    constexpr i64 minI64 = std::numeric_limits<i64>::min();
    Value v(minI64);
    EXPECT_EQ(v.asI64(), minI64);
}

// i64 最大值边界保持
TEST_F(ValueI64Test, MaxBoundaryPreserved)
{
    constexpr i64 maxI64 = std::numeric_limits<i64>::max();
    Value v(maxI64);
    EXPECT_EQ(v.asI64(), maxI64);
}

// toI64 与 asI64 行为一致
TEST_F(ValueI64Test, ToI64EqualsAsI64)
{
    Value v(static_cast<i64>(-123456789012LL));
    EXPECT_EQ(v.toI64(), v.asI64());
}

// i64 字符串转换
TEST_F(ValueI64Test, ToStringUsesLongLongOverload)
{
    Value v(static_cast<i64>(9876543210LL));
    EXPECT_EQ(v.toString(), "9876543210");
}

// i64 从字符串解析（asI64 使用 std::stoll）
TEST_F(ValueI64Test, AsI64FromString)
{
    Value v(std::string("9223372036854775807")); // i64 max
    EXPECT_EQ(v.asI64(), std::numeric_limits<i64>::max());
}

// i64 从字符串解析负数
TEST_F(ValueI64Test, AsI64FromNegativeString)
{
    Value v(std::string("-9223372036854775808")); // i64 min
    EXPECT_EQ(v.asI64(), std::numeric_limits<i64>::min());
}

// i64 从无效字符串返回 0
TEST_F(ValueI64Test, AsI64FromInvalidStringReturnsZero)
{
    Value v(std::string("not_a_number"));
    EXPECT_EQ(v.asI64(), 0);
}

// i64 asBool 非零为 true
TEST_F(ValueI64Test, AsBoolNonZeroIsTrue)
{
    Value v(static_cast<i64>(-1));
    EXPECT_TRUE(v.asBool());
    Value zero(static_cast<i64>(0));
    EXPECT_FALSE(zero.asBool());
}

// i64 asF64 精确转换
TEST_F(ValueI64Test, AsF64ExactConversion)
{
    Value v(static_cast<i64>(123456789));
    EXPECT_DOUBLE_EQ(v.asF64(), 123456789.0);
}

// i64 asU32 经 i64 中转，避免经 i32 的符号问题
TEST_F(ValueI64Test, AsU32ForLargeI64)
{
    constexpr i64 val = 4000000000LL; // 大于 i32 max，小于 u32 max
    Value v(val);
    EXPECT_EQ(v.asU32(), static_cast<u32>(4000000000ULL));
}

// ==================== u32 原生支持测试 ====================

class ValueU32Test : public ::testing::Test {
protected:
    void SetUp() override {}
};

// u32 构造后类型为 Integer
TEST_F(ValueU32Test, ConstructorMarksAsInteger)
{
    Value v(static_cast<u32>(0));
    EXPECT_TRUE(v.isInteger());
}

// u32 最大值通过 asI64 读取无符号问题
TEST_F(ValueU32Test, MaxU32PreservedThroughAsI64)
{
    constexpr u32 maxU32 = std::numeric_limits<u32>::max(); // 4294967295
    Value v(maxU32);
    EXPECT_EQ(v.asI64(), static_cast<i64>(4294967295ULL));
}

// u32 最大值通过 asU32 读取无符号问题
TEST_F(ValueU32Test, MaxU32PreservedThroughAsU32)
{
    constexpr u32 maxU32 = std::numeric_limits<u32>::max();
    Value v(maxU32);
    EXPECT_EQ(v.asU32(), maxU32);
}

// u32 值大于 i32 max 时，asInteger 会窄化为负数（行为契约）
TEST_F(ValueU32Test, LargeU32NarrowedByAsInteger)
{
    constexpr u32 val = static_cast<u32>(std::numeric_limits<i32>::max()) + 1u; // 2147483648
    Value v(val);
    // 窄化为 i32 后变成负数
    EXPECT_LT(v.asInteger(), 0);
}

// u32 字符串转换
TEST_F(ValueU32Test, ToStringLargeValue)
{
    Value v(static_cast<u32>(4294967295u));
    EXPECT_EQ(v.toString(), "4294967295");
}

// u32 toU32 与 asU32 行为一致
TEST_F(ValueU32Test, ToU32EqualsAsU32)
{
    Value v(static_cast<u32>(123456));
    EXPECT_EQ(v.toU32(), v.asU32());
}

// ==================== f64 原生支持测试 ====================

class ValueF64Test : public ::testing::Test {
protected:
    void SetUp() override {}
};

// f64 构造后类型为 Float
TEST_F(ValueF64Test, ConstructorMarksAsFloat)
{
    Value v(0.0);
    EXPECT_TRUE(v.isFloat());
}

// f64 高精度值通过 asF64 读取无精度丢失
TEST_F(ValueF64Test, HighPrecisionPreservedThroughAsF64)
{
    constexpr f64 precise = 3.141592653589793; // f32 无法精确表示
    Value v(precise);
    EXPECT_DOUBLE_EQ(v.asF64(), precise);
}

// f64 高精度值通过 asFloat 会损失精度（行为契约）
TEST_F(ValueF64Test, HighPrecisionLostByAsFloat)
{
    constexpr f64 precise = 3.141592653589793;
    Value v(precise);
    // asF64 保持原值，asFloat 窄化为 f32 后不再等于原 f64 值
    EXPECT_DOUBLE_EQ(v.asF64(), precise);
    EXPECT_NE(static_cast<f64>(v.asFloat()), precise);
}

// f64 asF64 从字符串解析（std::stod）
TEST_F(ValueF64Test, AsF64FromString)
{
    Value v(std::string("3.141592653589793"));
    EXPECT_DOUBLE_EQ(v.asF64(), 3.141592653589793);
}

// f64 asF64 从无效字符串返回 0
TEST_F(ValueF64Test, AsF64FromInvalidStringReturnsZero)
{
    Value v(std::string("not_a_number"));
    EXPECT_DOUBLE_EQ(v.asF64(), 0.0);
}

// f64 toF64 与 asF64 行为一致
TEST_F(ValueF64Test, ToF64EqualsAsF64)
{
    Value v(2.718281828459045);
    EXPECT_DOUBLE_EQ(v.toF64(), v.asF64());
}

// f64 asBool 非零为 true
TEST_F(ValueF64Test, AsBoolNonZeroIsTrue)
{
    Value v(0.0001);
    EXPECT_TRUE(v.asBool());
    Value zero(0.0);
    EXPECT_FALSE(zero.asBool());
}

// f64 负数保持
TEST_F(ValueF64Test, NegativePreserved)
{
    Value v(-123.456);
    EXPECT_DOUBLE_EQ(v.asF64(), -123.456);
}

// f64 asI64 截断小数部分
TEST_F(ValueF64Test, AsI64TruncatesFraction)
{
    Value v(99.9);
    EXPECT_EQ(v.asI64(), 99);
    Value neg(-99.9);
    EXPECT_EQ(neg.asI64(), -99);
}

// f64 大值超出 i32 但在 i64 范围内，asI64 保持
TEST_F(ValueF64Test, LargeF64AsI64Preserved)
{
    Value v(5000000000.0); // 50 亿，超出 i32 范围
    EXPECT_EQ(v.asI64(), 5000000000LL);
}

// ==================== fromAny 原生支持测试 ====================

class ValueFromAnyWideTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// fromAny 对 i64 直接构造，无精度丢失
TEST_F(ValueFromAnyWideTest, I64FromAnyPreserved)
{
    constexpr i64 val = 9223372036854775806LL;
    auto v = Value::fromAny(std::any(val));
    ASSERT_TRUE(v.isInteger());
    EXPECT_EQ(v.asI64(), val);
}

// fromAny 对 u32 直接构造，无符号问题
TEST_F(ValueFromAnyWideTest, U32FromAnyPreserved)
{
    constexpr u32 val = 4000000000u;
    auto v = Value::fromAny(std::any(val));
    ASSERT_TRUE(v.isInteger());
    EXPECT_EQ(v.asU32(), val);
    EXPECT_EQ(v.asI64(), static_cast<i64>(val));
}

// fromAny 对 f64 直接构造，无精度丢失
TEST_F(ValueFromAnyWideTest, F64FromAnyPreserved)
{
    constexpr f64 val = 3.141592653589793;
    auto v = Value::fromAny(std::any(val));
    ASSERT_TRUE(v.isFloat());
    EXPECT_DOUBLE_EQ(v.asF64(), val);
}

// fromAny 对 i32 仍然兼容
TEST_F(ValueFromAnyWideTest, I32FromAnyStillWorks)
{
    auto v = Value::fromAny(std::any(static_cast<i32>(42)));
    ASSERT_TRUE(v.isInteger());
    EXPECT_EQ(v.asInteger(), 42);
    EXPECT_EQ(v.asI64(), 42);
}

// fromAny 对 f32 仍然兼容
TEST_F(ValueFromAnyWideTest, F32FromAnyStillWorks)
{
    auto v = Value::fromAny(std::any(3.14f));
    ASSERT_TRUE(v.isFloat());
    EXPECT_FLOAT_EQ(v.asFloat(), 3.14f);
    EXPECT_DOUBLE_EQ(v.asF64(), static_cast<f64>(3.14f));
}

// ==================== operator== 跨类型比较测试 ====================

class ValueEqualityWideTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// i64 与 i32 同值时相等
TEST_F(ValueEqualityWideTest, I64EqualsI32SameValue)
{
    Value i32Val(static_cast<i32>(42));
    Value i64Val(static_cast<i64>(42));
    EXPECT_TRUE(i32Val == i64Val);
}

// i64 与 f64 同值时相等（跨类型，用 f64 比较）
TEST_F(ValueEqualityWideTest, I64EqualsF64SameValue)
{
    Value i64Val(static_cast<i64>(42));
    Value f64Val(42.0);
    EXPECT_TRUE(i64Val == f64Val);
}

// 大 i64 与 f64 跨类型比较，避免 f32 精度丢失
TEST_F(ValueEqualityWideTest, LargeI64EqualsF64WithoutPrecisionLoss)
{
    constexpr i64 large = 2147483648LL; // 2^31，f32 无法精确表示
    Value i64Val(large);
    Value f64Val(static_cast<f64>(large));
    // 用 f64 比较应当相等；若用 f32 比较会因精度丢失而不等
    EXPECT_TRUE(i64Val == f64Val);
}

// 不同值不相等
TEST_F(ValueEqualityWideTest, DifferentValuesNotEqual)
{
    Value i64Val(static_cast<i64>(100));
    Value f64Val(99.0);
    EXPECT_FALSE(i64Val == f64Val);
    EXPECT_TRUE(i64Val != f64Val);
}

// u32 与 i32 同值时相等
TEST_F(ValueEqualityWideTest, U32EqualsI32SameValue)
{
    Value u32Val(static_cast<u32>(42));
    Value i32Val(static_cast<i32>(42));
    EXPECT_TRUE(u32Val == i32Val);
}

// ==================== exposeWritable 写回精度测试 ====================

class ValueExposeWritableWideTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 使用单例，每个用例前清空状态
        mc::client::ui::kagero::state::StateStore::instance().clear();
        mc::client::ui::kagero::state::StateStore::instance().clearMiddlewares();
        mc::client::ui::kagero::event::EventBus::instance().clear();
    }

    void TearDown() override
    {
        mc::client::ui::kagero::state::StateStore::instance().clear();
        mc::client::ui::kagero::state::StateStore::instance().clearMiddlewares();
        mc::client::ui::kagero::event::EventBus::instance().clear();
    }
};

// exposeWritable 对 i64 目标使用 toI64，无精度丢失
TEST_F(ValueExposeWritableWideTest, I64TargetUsesToI64)
{
    i64 target = 0;
    BindingContext ctx(
        mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());
    ctx.exposeWritable<i64>("v", &target);

    constexpr i64 large = 9223372036854775806LL;
    EXPECT_TRUE(ctx.setBinding("v", Value(large)));
    EXPECT_EQ(target, large);
}

// exposeWritable 对 u32 目标使用 toI64 再窄化，大值无符号问题
TEST_F(ValueExposeWritableWideTest, U32TargetHandlesLargeValue)
{
    u32 target = 0;
    BindingContext ctx(
        mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());
    ctx.exposeWritable<u32>("v", &target);

    constexpr u32 large = 4000000000u;
    EXPECT_TRUE(ctx.setBinding("v", Value(large)));
    EXPECT_EQ(target, large);
}

// exposeWritable 对 f64 目标使用 toF64，无精度丢失
TEST_F(ValueExposeWritableWideTest, F64TargetUsesToF64)
{
    f64 target = 0.0;
    BindingContext ctx(
        mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());
    ctx.exposeWritable<f64>("v", &target);

    constexpr f64 precise = 3.141592653589793;
    EXPECT_TRUE(ctx.setBinding("v", Value(precise)));
    EXPECT_DOUBLE_EQ(target, precise);
}

// exposeWritable 对 i32 目标仍然兼容（经 toInteger 中转）
TEST_F(ValueExposeWritableWideTest, I32TargetStillWorks)
{
    i32 target = 0;
    BindingContext ctx(
        mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());
    ctx.exposeWritable<i32>("v", &target);

    EXPECT_TRUE(ctx.setBinding("v", Value(static_cast<i32>(123))));
    EXPECT_EQ(target, 123);
}

// exposeWritable 对 f32 目标仍然兼容（经 toFloat 中转）
TEST_F(ValueExposeWritableWideTest, F32TargetStillWorks)
{
    f32 target = 0.0f;
    BindingContext ctx(
        mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());
    ctx.exposeWritable<f32>("v", &target);

    EXPECT_TRUE(ctx.setBinding("v", Value(3.14f)));
    EXPECT_FLOAT_EQ(target, 3.14f);
}

// ==================== 综合边界测试 ====================

class ValueWideBoundaryTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// i64 max 边界完整保持
TEST_F(ValueWideBoundaryTest, I64MaxRoundTrip)
{
    constexpr i64 maxVal = std::numeric_limits<i64>::max();
    Value v(maxVal);
    EXPECT_EQ(v.asI64(), maxVal);
    EXPECT_EQ(v.toString(), "9223372036854775807");
}

// i64 min 边界完整保持
TEST_F(ValueWideBoundaryTest, I64MinRoundTrip)
{
    constexpr i64 minVal = std::numeric_limits<i64>::min();
    Value v(minVal);
    EXPECT_EQ(v.asI64(), minVal);
    EXPECT_EQ(v.toString(), "-9223372036854775808");
}

// u32 max 边界完整保持
TEST_F(ValueWideBoundaryTest, U32MaxRoundTrip)
{
    constexpr u32 maxVal = std::numeric_limits<u32>::max();
    Value v(maxVal);
    EXPECT_EQ(v.asU32(), maxVal);
    EXPECT_EQ(v.toString(), "4294967295");
}

// f64 高精度边界保持
TEST_F(ValueWideBoundaryTest, F64PrecisionRoundTrip)
{
    constexpr f64 precise = 1.0 / 3.0; // 无限循环，f64 精度内
    Value v(precise);
    EXPECT_DOUBLE_EQ(v.asF64(), precise);
}
