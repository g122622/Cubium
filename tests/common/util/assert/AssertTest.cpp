#include "common/util/assert/AssertAll.hpp"
#include <sstream>
#include <stdexcept>
#include <gtest/gtest.h>

using namespace mc::assert;

// ============================================================================
// 测试夹具
// ============================================================================

class AssertTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 保存原始配置
        originalConfig_ = AssertManager::instance().config();
    }

    void TearDown() override
    {
        // 恢复原始配置
        AssertManager::instance().setConfig(originalConfig_);
    }

    AssertConfig originalConfig_;
};

// ============================================================================
// AssertManager 测试
// ============================================================================

TEST_F(AssertTest, AssertManagerSingleton)
{
    auto& instance1 = AssertManager::instance();
    auto& instance2 = AssertManager::instance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(AssertTest, AssertManagerConfig)
{
    AssertConfig config;
    config.captureStackTrace = true;
    config.breakOnFailure = false;

    AssertManager::instance().setConfig(config);

    const auto& currentConfig = AssertManager::instance().config();
    EXPECT_TRUE(currentConfig.captureStackTrace);
    EXPECT_FALSE(currentConfig.breakOnFailure);
}

// ============================================================================
// AssertException 测试
// ============================================================================

TEST_F(AssertTest, AssertExceptionBasic)
{
    AssertFailure failure;
    failure.expression = "x == 42";
    failure.message = "Expected x to be 42";
    failure.file = "test.cpp";
    failure.line = 100;
    failure.function = "testFunction";
    failure.level = AssertLevel::Debug;

    AssertException ex(failure);

    EXPECT_EQ(failure.expression, ex.failure().expression);
    EXPECT_EQ(failure.message, ex.failure().message);
    EXPECT_EQ(failure.file, ex.failure().file);
    EXPECT_EQ(failure.line, ex.failure().line);
    EXPECT_EQ(failure.function, ex.failure().function);

    std::string what = ex.what();
    EXPECT_NE(what.find("x == 42"), std::string::npos);
    EXPECT_NE(what.find("test.cpp"), std::string::npos);
}

// ============================================================================
// 自定义处理器测试
// ============================================================================

TEST_F(AssertTest, CustomHandlerCalled)
{
    bool handlerCalled = false;
    std::string capturedExpression;

    AssertConfig config;
    config.handler = [&](const AssertFailure& failure) {
        handlerCalled = true;
        capturedExpression = failure.expression;
        throw std::runtime_error("Test assertion failed");
    };
    config.throwException = true;

    AssertManager::instance().setConfig(config);

    EXPECT_THROW(
        {
            AssertManager::instance().handleFailure(
                "test_expr", "test_msg", "test.cpp", 1, "testFunc", AssertLevel::Debug);
        },
        std::runtime_error);

    EXPECT_TRUE(handlerCalled);
    EXPECT_EQ("test_expr", capturedExpression);
}

TEST_F(AssertTest, ThrowAssertHandler)
{
    EXPECT_THROW(
        { throwAssertHandler(AssertFailure{"expr", "msg", "file.cpp", 10, "func", AssertLevel::Debug, ""}); },
        AssertException);
}

// ============================================================================
// 格式化帮助函数测试
// ============================================================================

TEST_F(AssertTest, FormatValueInt)
{
    EXPECT_EQ("42", detail::formatValue(42));
    EXPECT_EQ("-123", detail::formatValue(-123));
}

TEST_F(AssertTest, FormatValueFloat)
{
    EXPECT_EQ("3.14", detail::formatValue(3.14));
}

TEST_F(AssertTest, FormatValueBool)
{
    EXPECT_EQ("true", detail::formatValue(true));
    EXPECT_EQ("false", detail::formatValue(false));
}

TEST_F(AssertTest, FormatValueCString)
{
    EXPECT_EQ("nullptr", detail::formatValue(static_cast<const char*>(nullptr)));
    EXPECT_EQ("\"hello\"", detail::formatValue("hello"));
}

TEST_F(AssertTest, FormatValueString)
{
    EXPECT_EQ("\"test string\"", detail::formatValue(std::string("test string")));
}

TEST_F(AssertTest, FormatValuePointer)
{
    int x = 42;
    std::string result = detail::formatValue(&x);
    // 指针格式化结果不应该为空
    EXPECT_FALSE(result.empty());
    EXPECT_NE("nullptr", result);

    int* nullPtr = nullptr;
    EXPECT_EQ("nullptr", detail::formatValue(nullPtr));
}

TEST_F(AssertTest, FormatComparisonMessage)
{
    std::string msg = detail::formatComparisonMessage("not equal", "a", 42, "b", 100);

    EXPECT_NE(msg.find("not equal"), std::string::npos);
    EXPECT_NE(msg.find("a = 42"), std::string::npos);
    EXPECT_NE(msg.find("b = 100"), std::string::npos);
}

// ============================================================================
// 宏测试 (使用 Release 级别断言，在所有构建中都启用)
// ============================================================================

class MacroTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        originalConfig_ = AssertManager::instance().config();

        AssertConfig config;
        config.handler = [this](const AssertFailure& failure) {
            lastFailure_ = failure;
            failureCount_++;
            throw AssertException(failure);
        };
        AssertManager::instance().setConfig(config);

        failureCount_ = 0;
    }

    void TearDown() override { AssertManager::instance().setConfig(originalConfig_); }

    AssertConfig originalConfig_;
    AssertFailure lastFailure_;
    int failureCount_ = 0;
};

// ============================================================================
// MC_ASSERT_RELEASE 测试 - 在所有构建中都启用
// ============================================================================

TEST_F(MacroTest, MC_ASSERT_RELEASE_Passes)
{
    EXPECT_NO_THROW({ MC_ASSERT_RELEASE(true); });
    EXPECT_EQ(0, failureCount_);
}

TEST_F(MacroTest, MC_ASSERT_RELEASE_Fails)
{
    EXPECT_THROW({ MC_ASSERT_RELEASE(false); }, AssertException);
    EXPECT_EQ(1, failureCount_);
}

TEST_F(MacroTest, MC_ASSERT_RELEASE_MSG_Passes)
{
    EXPECT_NO_THROW({ MC_ASSERT_RELEASE_MSG(true, "Should not fail"); });
    EXPECT_EQ(0, failureCount_);
}

TEST_F(MacroTest, MC_ASSERT_RELEASE_MSG_Fails)
{
    EXPECT_THROW({ MC_ASSERT_RELEASE_MSG(false, "Custom message"); }, AssertException);

    EXPECT_EQ(1, failureCount_);
    EXPECT_EQ("Custom message", lastFailure_.message);
}

// ============================================================================
// MC_ASSERT_FATAL 测试 - 在所有构建中都启用
// ============================================================================

TEST_F(MacroTest, MC_ASSERT_FATAL_Passes)
{
    EXPECT_NO_THROW({ MC_ASSERT_FATAL(true); });
}

TEST_F(MacroTest, MC_ASSERT_FATAL_Fails)
{
    EXPECT_THROW({ MC_ASSERT_FATAL(false); }, AssertException);
    EXPECT_EQ(AssertLevel::Fatal, lastFailure_.level);
}

TEST_F(MacroTest, MC_ASSERT_FATAL_MSG_Fails)
{
    EXPECT_THROW({ MC_ASSERT_FATAL_MSG(false, "Fatal error"); }, AssertException);
    EXPECT_EQ("Fatal error", lastFailure_.message);
    EXPECT_EQ(AssertLevel::Fatal, lastFailure_.level);
}

// ============================================================================
// 指针断言测试
// ============================================================================

TEST_F(MacroTest, MC_ASSERT_NULL_RELEASE_Passes)
{
    int* ptr = nullptr;
    EXPECT_NO_THROW({ MC_ASSERT_NULL_RELEASE(ptr); });
}

TEST_F(MacroTest, MC_ASSERT_NULL_RELEASE_Fails)
{
    int x = 42;
    EXPECT_THROW({ MC_ASSERT_NULL_RELEASE(&x); }, AssertException);
}

TEST_F(MacroTest, MC_ASSERT_NOT_NULL_RELEASE_Passes)
{
    int x = 42;
    EXPECT_NO_THROW({ MC_ASSERT_NOT_NULL_RELEASE(&x); });
}

TEST_F(MacroTest, MC_ASSERT_NOT_NULL_RELEASE_Fails)
{
    int* ptr = nullptr;
    EXPECT_THROW({ MC_ASSERT_NOT_NULL_RELEASE(ptr); }, AssertException);
}

// ============================================================================
// 比较断言测试（带值输出）
// ============================================================================

TEST_F(MacroTest, MC_ASSERT_EQ_Passes)
{
    EXPECT_NO_THROW({ MC_ASSERT_EQ(42, 42); });
    EXPECT_NO_THROW({ MC_ASSERT_EQ(std::string("hello"), std::string("hello")); });
}

TEST_F(MacroTest, MC_ASSERT_EQ_Fails)
{
    EXPECT_THROW({ MC_ASSERT_EQ(42, 100); }, AssertException);

    // 检查失败消息包含值
    EXPECT_NE(lastFailure_.message.find("42"), std::string::npos);
    EXPECT_NE(lastFailure_.message.find("100"), std::string::npos);
}

TEST_F(MacroTest, MC_ASSERT_NE_Passes)
{
    EXPECT_NO_THROW({ MC_ASSERT_NE(42, 100); });
}

TEST_F(MacroTest, MC_ASSERT_NE_Fails)
{
    EXPECT_THROW({ MC_ASSERT_NE(42, 42); }, AssertException);
}

TEST_F(MacroTest, MC_ASSERT_LT_Passes)
{
    EXPECT_NO_THROW({ MC_ASSERT_LT(1, 2); });
}

TEST_F(MacroTest, MC_ASSERT_LT_Fails)
{
    EXPECT_THROW({ MC_ASSERT_LT(2, 1); }, AssertException);
    EXPECT_THROW({ MC_ASSERT_LT(1, 1); }, AssertException);
}

TEST_F(MacroTest, MC_ASSERT_LE_Passes)
{
    EXPECT_NO_THROW({ MC_ASSERT_LE(1, 2); });
    EXPECT_NO_THROW({ MC_ASSERT_LE(1, 1); });
}

TEST_F(MacroTest, MC_ASSERT_LE_Fails)
{
    EXPECT_THROW({ MC_ASSERT_LE(2, 1); }, AssertException);
}

TEST_F(MacroTest, MC_ASSERT_GT_Passes)
{
    EXPECT_NO_THROW({ MC_ASSERT_GT(2, 1); });
}

TEST_F(MacroTest, MC_ASSERT_GT_Fails)
{
    EXPECT_THROW({ MC_ASSERT_GT(1, 2); }, AssertException);
    EXPECT_THROW({ MC_ASSERT_GT(1, 1); }, AssertException);
}

TEST_F(MacroTest, MC_ASSERT_GE_Passes)
{
    EXPECT_NO_THROW({ MC_ASSERT_GE(2, 1); });
    EXPECT_NO_THROW({ MC_ASSERT_GE(1, 1); });
}

TEST_F(MacroTest, MC_ASSERT_GE_Fails)
{
    EXPECT_THROW({ MC_ASSERT_GE(1, 2); }, AssertException);
}

// ============================================================================
// 特殊断言测试
// ============================================================================

TEST_F(MacroTest, MC_ASSERT_UNREACHABLE)
{
    EXPECT_THROW({ MC_ASSERT_UNREACHABLE(); }, AssertException);
    EXPECT_EQ(AssertLevel::Fatal, lastFailure_.level);
    EXPECT_NE(lastFailure_.expression.find("unreachable"), std::string::npos);
}

TEST_F(MacroTest, MC_ASSERT_UNREACHABLE_MSG)
{
    EXPECT_THROW({ MC_ASSERT_UNREACHABLE_MSG("This code path"); }, AssertException);
    EXPECT_EQ(AssertLevel::Fatal, lastFailure_.level);
    EXPECT_NE(lastFailure_.message.find("This code path"), std::string::npos);
}

TEST_F(MacroTest, MC_ASSERT_FAIL)
{
    EXPECT_THROW({ MC_ASSERT_FAIL("This is a test failure"); }, AssertException);
    EXPECT_EQ(AssertLevel::Fatal, lastFailure_.level);
    EXPECT_EQ("This is a test failure", lastFailure_.message);
}

TEST_F(MacroTest, MC_ASSERT_NOT_IMPLEMENTED)
{
    EXPECT_THROW({ MC_ASSERT_NOT_IMPLEMENTED(); }, AssertException);
    EXPECT_EQ(AssertLevel::Fatal, lastFailure_.level);
    // 消息是函数名
    EXPECT_FALSE(lastFailure_.message.empty());
}

// ============================================================================
// MC_UNUSED 测试
// ============================================================================

TEST_F(MacroTest, MC_UNUSED)
{
    int unused = 42;
    // 不应该产生警告
    MC_UNUSED(unused);
    (void)unused; // 避免编译器警告
}

// ============================================================================
// Debug 模式测试 - 仅在 Debug 模式下运行
// ============================================================================

#ifndef NDEBUG

TEST_F(MacroTest, MC_ASSERT_Passes_Debug)
{
    EXPECT_NO_THROW({ MC_ASSERT(true); });
    EXPECT_EQ(0, failureCount_);
}

TEST_F(MacroTest, MC_ASSERT_Fails_Debug)
{
    EXPECT_THROW({ MC_ASSERT(false); }, AssertException);
    EXPECT_EQ(1, failureCount_);
    EXPECT_EQ("false", lastFailure_.expression);
}

TEST_F(MacroTest, MC_ASSERT_MSG_Fails_Debug)
{
    EXPECT_THROW({ MC_ASSERT_MSG(false, "Custom message"); }, AssertException);
    EXPECT_EQ(1, failureCount_);
    EXPECT_EQ("Custom message", lastFailure_.message);
}

#endif // NDEBUG

// ============================================================================
// 堆栈跟踪测试
// ============================================================================

TEST_F(AssertTest, CaptureStackTrace)
{
    AssertConfig config;
    config.captureStackTrace = true;
    AssertManager::instance().setConfig(config);

    std::string stackTrace = AssertManager::instance().captureStackTrace();

    // 堆栈跟踪应该包含一些信息
    // 注意：具体内容取决于平台和编译器优化
    EXPECT_FALSE(stackTrace.empty());
}

TEST_F(AssertTest, NoStackTraceWhenDisabled)
{
    AssertConfig config;
    config.captureStackTrace = false;
    AssertManager::instance().setConfig(config);

    config.handler = [&](const AssertFailure& failure) {
        EXPECT_TRUE(failure.stackTrace.empty());
        throw AssertException(failure);
    };
    AssertManager::instance().setConfig(config);

    EXPECT_THROW(
        { AssertManager::instance().handleFailure("test", nullptr, "test.cpp", 1, "testFunc", AssertLevel::Debug); },
        AssertException);
}

// ============================================================================
// 集成测试
// ============================================================================

TEST_F(MacroTest, MultipleAssertions)
{
    // 测试多个连续断言
    int count = 0;

    auto testFunc = [&]() {
        MC_ASSERT_RELEASE(true); // 通过
        count++;
        MC_ASSERT_RELEASE_MSG(true, "Should pass"); // 通过
        count++;
        MC_ASSERT_EQ(1, 1); // 通过
        count++;
    };

    EXPECT_NO_THROW(testFunc());
    EXPECT_EQ(3, count);
}

TEST_F(MacroTest, AssertInLambda)
{
    auto failingLambda = []() { MC_ASSERT_RELEASE(false); };

    EXPECT_THROW(failingLambda(), AssertException);
}

TEST_F(MacroTest, AssertInNestedFunction)
{
    auto outerFunc = []() {
        auto innerFunc = []() { MC_ASSERT_RELEASE_MSG(false, "Inner function failed"); };
        innerFunc();
    };

    EXPECT_THROW(outerFunc(), AssertException);
    EXPECT_EQ("Inner function failed", lastFailure_.message);
}

// ============================================================================
// 性能测试（可选）
// ============================================================================

TEST_F(MacroTest, AssertPerformance)
{
    // 确保断言通过时没有显著性能开销
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000000; ++i) {
        MC_ASSERT_RELEASE(true);
        MC_ASSERT_NOT_NULL_RELEASE(&i);
        MC_ASSERT_EQ(i, i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 100万次断言应该很快（这里只是粗略检查）
    EXPECT_LT(duration.count(), 1000); // 应该小于1秒
}

// ============================================================================
// 默认处理器测试
// ============================================================================

TEST_F(AssertTest, DefaultHandlerFormat)
{
    std::string message =
        detail::formatFailureMessage("x == 42", "Expected x to be 42", "test.cpp", 100, "testFunction");

    EXPECT_NE(message.find("x == 42"), std::string::npos);
    EXPECT_NE(message.find("Expected x to be 42"), std::string::npos);
    EXPECT_NE(message.find("test.cpp"), std::string::npos);
    EXPECT_NE(message.find("100"), std::string::npos);
    EXPECT_NE(message.find("testFunction"), std::string::npos);
}
