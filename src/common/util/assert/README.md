# Assert Library

一个功能强大的运行时断言库，支持多种断言级别、堆栈跟踪和自定义处理器。

## 目录结构

```
src/common/util/assert/
├── Assert.hpp           # 核心类和接口定义
├── Assert.cpp           # 实现文件（跨平台堆栈跟踪）
├── AssertMacros.hpp     # 断言宏定义
├── AssertAll.hpp        # 统一包含入口
└── README.md            # 本文档
```

## 文件详细介绍

### Assert.hpp - 核心头文件

**职责**：定义断言库的核心类型、接口和管理器。

**主要内容**：

| 类型 | 说明 |
|------|------|
| `AssertLevel` | 断言级别枚举（Debug/Release/Fatal） |
| `AssertFailure` | 断言失败信息结构体 |
| `AssertHandler` | 自定义处理器函数类型 |
| `AssertConfig` | 断言配置结构体 |
| `AssertManager` | 单例管理器类 |
| `AssertException` | 断言异常类 |
| `detail::formatValue<T>()` | 值格式化帮助函数（模板） |
| `detail::formatComparisonMessage<T, U>()` | 比较断言消息格式化 |

**关键接口**：

```cpp
// AssertManager 单例管理器
class AssertManager {
public:
    static AssertManager& instance();              // 获取单例
    void setConfig(const AssertConfig& config);    // 设置配置
    const AssertConfig& config() const;            // 获取配置
    void setHandler(AssertHandler handler);        // 设置处理器
    [[noreturn]] void handleFailure(...);          // 处理断言失败
    bool handleRecoverableFailure(...);            // 处理可恢复失败
    String captureStackTrace() const;              // 捕获堆栈跟踪
};

// AssertFailure 断言失败信息
struct AssertFailure {
    String expression;      // 断言表达式
    String message;         // 自定义消息
    String file;            // 文件名
    i32 line;               // 行号
    String function;        // 函数名
    AssertLevel level;      // 断言级别
    String stackTrace;      // 堆栈跟踪（可选）
};

// AssertConfig 配置选项
struct AssertConfig {
    AssertHandler handler;          // 自定义处理器
    bool captureStackTrace = false; // 是否捕获堆栈跟踪
    bool breakOnFailure = false;    // 是否触发调试器断点
    bool throwException = false;    // 是否抛出异常
    bool continueExecution = false; // 是否继续执行（仅 Debug 级别）
};
```

### Assert.cpp - 实现文件

**职责**：实现 `AssertManager` 和处理器函数，提供跨平台堆栈跟踪。

**主要内容**：

| 函数 | 说明 |
|------|------|
| `AssertManager::instance()` | 单例获取 |
| `AssertManager::setConfig()` | 配置设置 |
| `AssertManager::handleFailure()` | 失败处理（不可恢复） |
| `AssertManager::handleRecoverableFailure()` | 失败处理（可恢复） |
| `AssertManager::captureStackTrace()` | 跨平台堆栈跟踪捕获 |
| `defaultAssertHandler()` | 默认处理器（stderr + abort） |
| `logAssertHandler()` | 日志处理器（spdlog + abort） |
| `throwAssertHandler()` | 异常处理器（抛出 AssertException） |
| `AssertException::AssertException()` | 异常构造 |
| `detail::formatFailureMessage()` | 失败消息格式化 |

**跨平台堆栈跟踪支持**：

- **Windows**：使用 `CaptureStackBackTrace` + `SymFromAddr` + `SymGetLineFromAddr64`
- **Linux/macOS**：使用 `backtrace` + `backtrace_symbols` + `abi::__cxa_demangle`

### AssertMacros.hpp - 断言宏定义

**职责**：提供各种断言宏，简化断言使用。

**宏分类**：

| 类别 | 宏 | 说明 |
|------|-----|------|
| **基本断言** | `MC_ASSERT(cond)` | Debug 模式断言 |
| | `MC_ASSERT_MSG(cond, msg)` | 带消息的 Debug 断言 |
| | `MC_ASSERT_DEBUG(cond)` | 明确标记为 Debug 级别 |
| **Release 断言** | `MC_ASSERT_RELEASE(cond)` | Release 模式也启用 |
| | `MC_ASSERT_RELEASE_MSG(cond, msg)` | 带消息的 Release 断言 |
| **致命断言** | `MC_ASSERT_FATAL(cond)` | 致命错误，始终启用 |
| | `MC_ASSERT_FATAL_MSG(cond, msg)` | 带消息的致命断言 |
| **指针断言** | `MC_ASSERT_NULL(ptr)` | 检查指针为空 |
| | `MC_ASSERT_NOT_NULL(ptr)` | 检查指针非空 |
| | `MC_ASSERT_NULL_RELEASE(ptr)` | Release 模式指针空检查 |
| | `MC_ASSERT_NOT_NULL_RELEASE(ptr)` | Release 模式指针非空检查 |
| **范围断言** | `MC_ASSERT_RANGE(val, min, max)` | 检查值在闭区间内 |
| | `MC_ASSERT_RANGE_EXCLUSIVE(val, min, max)` | 检查值在开区间内 |
| | `MC_ASSERT_INDEX(idx, size)` | 检查有符号索引有效 |
| | `MC_ASSERT_INDEX_U(idx, size)` | 检查无符号索引有效 |
| **比较断言** | `MC_ASSERT_EQ(a, b)` | 相等断言（输出两值） |
| | `MC_ASSERT_NE(a, b)` | 不相等断言 |
| | `MC_ASSERT_LT(a, b)` | 小于断言 |
| | `MC_ASSERT_LE(a, b)` | 小于等于断言 |
| | `MC_ASSERT_GT(a, b)` | 大于断言 |
| | `MC_ASSERT_GE(a, b)` | 大于等于断言 |
| **特殊断言** | `MC_ASSERT_UNREACHABLE()` | 标记不可达代码 |
| | `MC_ASSERT_UNREACHABLE_MSG(msg)` | 带消息的不可达断言 |
| | `MC_ASSERT_FAIL(msg)` | 总是失败 |
| | `MC_ASSERT_NOT_IMPLEMENTED()` | 标记未实现功能 |
| **条件断言** | `MC_PRECONDITION(cond)` | 前置条件（函数入口） |
| | `MC_POSTCONDITION(cond)` | 后置条件（函数出口） |
| | `MC_INVARIANT(cond)` | 不变量（对象状态） |
| **调试辅助** | `MC_DEBUG_ONLY(expr)` | 仅 Debug 模式执行 |
| | `MC_UNUSED(var)` | 标记未使用变量 |

### AssertAll.hpp - 统一包含入口

**职责**：提供统一入口，简化头文件包含。

**内容**：

```cpp
#pragma once

#include "Assert.hpp"
#include "AssertMacros.hpp"
```

**使用方式**：

```cpp
#include "common/util/assert/AssertAll.hpp"  // 包含所有断言功能
```

## 文件关系图

```
                    ┌─────────────────┐
                    │  AssertAll.hpp  │  ← 用户包含入口
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              │              ▼
      ┌───────────────┐      │      ┌────────────────┐
      │  Assert.hpp   │──────┼─────▶│ AssertMacros.hpp│
      └───────┬───────┘      │      └────────────────┘
              │              │              │
              │              │              │
              │              │              │ 包含 Assert.hpp
              │              │              │ 使用 AssertManager
              │              │              │
              ▼              │              │
      ┌───────────────┐      │              │
      │  Assert.cpp   │◀─────┘              │
      └───────────────┘  实现 Assert.hpp    │
                         被宏调用           │
                                            │
```

## 整体职责

Assert 库提供运行时断言功能，用于：

1. **开发调试**：通过 Debug 级别断言捕获逻辑错误
2. **安全检查**：通过 Release 级别断言验证关键条件
3. **错误处理**：通过 Fatal 级别断言处理不可恢复错误
4. **调试支持**：提供堆栈跟踪、调试器断点等功能
5. **自定义处理**：支持自定义断言失败处理器

## 输入和输出

### 输入

| 输入类型 | 来源 | 说明 |
|---------|------|------|
| 断言条件 | 宏参数 | 用户代码中的条件表达式 |
| 自定义消息 | 宏参数 | 可选的错误描述 |
| 配置选项 | `AssertConfig` | 堆栈跟踪、断点、处理器等 |
| 处理器函数 | `AssertHandler` | 自定义失败处理逻辑 |

### 输出

| 输出类型 | 形式 | 说明 |
|---------|------|------|
| 错误消息 | stderr 输出 | 格式化的断言失败信息 |
| 堆栈跟踪 | 可选输出 | 调用栈信息（需配置） |
| 异常抛出 | `AssertException` | 可选的异常处理方式 |
| 程序终止 | `std::abort()` | 默认行为 |

## 依赖项

### 内部依赖

| 依赖 | 说明 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义（`i32`, `u8`, `String` 等） |

### 外部依赖

| 依赖 | 说明 |
|------|------|
| `<string>` | 标准字符串 |
| `<functional>` | 函数对象（`std::function`） |
| `<sstream>` | 字符串流 |
| `<vector>` | 动态数组 |
| `<iostream>` | 输入输出流 |
| `<cstdlib>` | `std::abort()` |
| `<exception>` | 异常基类 |

### 平台特定依赖

| 平台 | 依赖 |
|------|------|
| Windows | `Windows.h`, `DbgHelp.h`, `dbghelp.lib` |
| Linux | `execinfo.h`, `cxxabi.h` |
| macOS | `execinfo.h`, `cxxabi.h` |

## 使用方法

### 快速入门

```cpp
#include "common/util/assert/AssertAll.hpp"

// 基本断言（仅 Debug 模式）
MC_ASSERT(ptr != nullptr);
MC_ASSERT_MSG(size > 0, "Size must be positive");

// Release 模式断言（始终启用）
MC_ASSERT_RELEASE(index < capacity);
MC_ASSERT_RELEASE_MSG(result != nullptr, "Result cannot be null");

// 致命断言（始终启用，用于不可恢复的错误）
MC_ASSERT_FATAL(state == State::Ready);
MC_ASSERT_FATAL_MSG(initialized, "System not initialized");
```

### 比较断言

```cpp
// 比较断言会输出两个值的实际内容
int expected = 42;
int actual = 100;

MC_ASSERT_EQ(expected, actual);   // 失败时显示: expected = 42, actual = 100
MC_ASSERT_NE(a, b);               // 不相等断言
MC_ASSERT_LT(a, b);               // 小于断言
MC_ASSERT_LE(a, b);               // 小于等于断言
MC_ASSERT_GT(a, b);               // 大于断言
MC_ASSERT_GE(a, b);               // 大于等于断言
```

### 指针和范围断言

```cpp
// 指针断言
int* ptr = nullptr;
MC_ASSERT_NULL(ptr);              // 检查指针为空
MC_ASSERT_NOT_NULL(&value);       // 检查指针非空

// 范围断言
MC_ASSERT_RANGE(index, 0, size - 1);     // 0 <= index <= size-1
MC_ASSERT_INDEX(row, height);            // 0 <= row < height
MC_ASSERT_INDEX_U(uindex, size);         // uindex < size（无符号）
```

### 特殊断言

```cpp
// 标记不可达代码
switch (value) {
    case 0: /* ... */ break;
    case 1: /* ... */ break;
    default:
        MC_ASSERT_UNREACHABLE();  // 永远不应该到达这里
}

// 总是失败
MC_ASSERT_FAIL("Critical error: database connection lost");

// 标记未实现功能
MC_ASSERT_NOT_IMPLEMENTED();  // 消息为当前函数名
```

### 前置/后置条件

```cpp
void processData(const std::vector<int>& data) {
    // 前置条件
    MC_PRECONDITION(!data.empty());
    MC_PRECONDITION_MSG(data.size() <= MAX_SIZE, "Data too large");

    // ... 处理数据 ...

    // 后置条件
    MC_POSTCONDITION(result != nullptr);
}

class Container {
    int m_count;
    void validate() {
        // 不变量检查
        MC_INVARIANT(m_count >= 0);
    }
};
```

### 自定义处理器

```cpp
void myAssertHandler(const mc::assert::AssertFailure& failure) {
    // 记录日志
    spdlog::error("Assertion failed: {} at {}:{}",
                  failure.expression, failure.file, failure.line);

    // 发送错误报告
    sendErrorReport(failure);

    // 抛出异常
    throw mc::assert::AssertException(failure);
}

int main() {
    mc::assert::AssertConfig config;
    config.handler = myAssertHandler;
    config.captureStackTrace = true;   // 启用堆栈跟踪
    config.breakOnFailure = true;      // 触发调试器断点

    mc::assert::AssertManager::instance().setConfig(config);

    // ... 应用程序代码 ...
}
```

### 调试辅助

```cpp
// 仅在 Debug 模式执行的代码
MC_DEBUG_ONLY(debugLog("Checking state..."));
MC_DEBUG_ONLY(validateInternalState());

// 标记未使用变量（避免编译器警告）
void callback(int value, void* userdata) {
    MC_UNUSED(userdata);
    process(value);
}
```

## 容易踩的坑

### 1. 断言中的副作用

**问题**：Debug 断言在 Release 模式下被禁用，其中的代码不会执行。

```cpp
// ❌ 错误：initialize() 在 Release 中不会被调用
MC_ASSERT(initialize());

// ✅ 正确：先执行，再断言
bool ok = initialize();
MC_ASSERT(ok);
```

### 2. 断言级别选择错误

| 级别 | Debug 构建 | Release 构建 | 适用场景 |
|------|-----------|-------------|---------|
| Debug | ✅ 启用 | ❌ 禁用 | 开发调试、内部不变量 |
| Release | ✅ 启用 | ✅ 启用 | 关键检查、边界验证 |
| Fatal | ✅ 启用 | ✅ 启用 | 不可恢复错误、程序状态严重错误 |

**建议**：
- 开发调试用 `MC_ASSERT`
- 安全关键检查用 `MC_ASSERT_RELEASE`
- 不可恢复错误用 `MC_ASSERT_FATAL`

### 3. 比较断言的类型要求

比较断言要求操作数支持对应的运算符：

```cpp
// 自定义类型需要定义运算符
struct Vec2 {
    int x, y;
    bool operator==(const Vec2& other) const {
        return x == other.x && y == other.y;
    }
    bool operator<(const Vec2& other) const {
        return x < other.x || (x == other.x && y < other.y);
    }
};

// 现在可以使用
MC_ASSERT_EQ(vec1, vec2);
MC_ASSERT_LT(vec1, vec2);
```

### 4. 多线程环境

断言管理器是单例，处理器应该是线程安全的：

```cpp
// ❌ 不安全：非线程安全的处理器
void unsafeHandler(const AssertFailure& failure) {
    static std::string buffer;  // 非线程安全
    buffer = failure.expression;
    log(buffer);
}

// ✅ 安全：线程安全的处理器
std::mutex g_logMutex;
void threadSafeHandler(const AssertFailure& failure) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    logToFile(failure);
}
```

### 5. 断言 vs 异常

- **断言**：用于检查程序内部逻辑错误（不可恢复）
- **异常**：用于处理可预期的错误条件（可恢复）

```cpp
// 断言：检查内部不变量（程序错误）
MC_ASSERT(m_count >= 0);

// 异常：处理外部输入（用户错误）
if (userInput < 0) {
    throw std::invalid_argument("Input must be positive");
}
```

### 6. 堆栈跟踪性能开销

启用堆栈跟踪有性能开销，建议仅在调试时启用：

```cpp
// 调试配置
#ifdef NDEBUG
    AssertConfig config;  // 生产环境不捕获堆栈跟踪
    config.captureStackTrace = false;
#else
    AssertConfig config;  // 开发环境捕获堆栈跟踪
    config.captureStackTrace = true;
#endif
```

### 7. Release 构建中的性能

即使在 Release 构建，`MC_ASSERT_RELEASE` 也会有轻微开销：

```cpp
// 热点代码路径中谨慎使用
for (int i = 0; i < millions; ++i) {
    // ❌ 可能影响性能
    MC_ASSERT_RELEASE(data[i] != nullptr);

    // ✅ 更高效的方式
    if (data[i] == nullptr) [[unlikely]] {
        handleNullCase();
    }
}
```

## 测试用例

测试文件位置：`tests/common/util/assert/AssertTest.cpp`

### 测试统计

| 测试套件 | 测试数量 |
|---------|---------|
| `AssertTest` | 15 个 |
| `MacroTest` | 35 个 |
| **总计** | **50 个** |

### 测试覆盖范围

#### AssertTest 测试套件

| 测试名称 | 覆盖内容 |
|---------|---------|
| `AssertManagerSingleton` | 单例模式验证 |
| `AssertManagerConfig` | 配置设置和获取 |
| `AssertExceptionBasic` | 异常构造和 `what()` 消息 |
| `CustomHandlerCalled` | 自定义处理器调用 |
| `ThrowAssertHandler` | 异常处理器 |
| `FormatValueInt` | 整数格式化 |
| `FormatValueFloat` | 浮点格式化 |
| `FormatValueBool` | 布尔格式化 |
| `FormatValueCString` | C 字符串格式化 |
| `FormatValueString` | String 格式化 |
| `FormatValuePointer` | 指针格式化 |
| `FormatComparisonMessage` | 比较消息格式化 |
| `CaptureStackTrace` | 堆栈跟踪捕获 |
| `NoStackTraceWhenDisabled` | 禁用堆栈跟踪 |
| `DefaultHandlerFormat` | 默认处理器格式 |

#### MacroTest 测试套件

| 测试名称 | 覆盖内容 |
|---------|---------|
| `MC_ASSERT_RELEASE_Passes/Fails` | Release 断言通过/失败 |
| `MC_ASSERT_RELEASE_MSG_Passes/Fails` | 带 Release 消息断言 |
| `MC_ASSERT_FATAL_Passes/Fails` | Fatal 断言通过/失败 |
| `MC_ASSERT_FATAL_MSG_Fails` | 带 Fatal 消息断言 |
| `MC_ASSERT_NULL_RELEASE_Passes/Fails` | 指针空断言 |
| `MC_ASSERT_NOT_NULL_RELEASE_Passes/Fails` | 指针非空断言 |
| `MC_ASSERT_EQ_Passes/Fails` | 相等断言 |
| `MC_ASSERT_NE_Passes/Fails` | 不相等断言 |
| `MC_ASSERT_LT_Passes/Fails` | 小于断言 |
| `MC_ASSERT_LE_Passes/Fails` | 小于等于断言 |
| `MC_ASSERT_GT_Passes/Fails` | 大于断言 |
| `MC_ASSERT_GE_Passes/Fails` | 大于等于断言 |
| `MC_ASSERT_UNREACHABLE` | 不可达断言 |
| `MC_ASSERT_UNREACHABLE_MSG` | 带消息不可达断言 |
| `MC_ASSERT_FAIL` | 总是失败断言 |
| `MC_ASSERT_NOT_IMPLEMENTED` | 未实现断言 |
| `MC_UNUSED` | 未使用变量标记 |
| `MC_ASSERT_Passes/Fails_Debug` | Debug 断言（仅 Debug 模式） |
| `MC_ASSERT_MSG_Fails_Debug` | Debug 消息断言（仅 Debug 模式） |
| `MultipleAssertions` | 多断言连续执行 |
| `AssertInLambda` | Lambda 中断言 |
| `AssertInNestedFunction` | 嵌套函数中断言 |
| `AssertPerformance` | 性能测试（100万次断言 < 1秒） |

### 运行测试

```powershell
# 运行所有断言测试
./build/bin/Release/mc_tests.exe --gtest_filter="*Assert*"

# 运行 AssertTest 测试套件
./build/bin/Release/mc_tests.exe --gtest_filter="AssertTest.*"

# 运行 MacroTest 测试套件
./build/bin/Release/mc_tests.exe --gtest_filter="MacroTest.*"
```

## 断言级别参考

| 级别 | 宏 | Debug 模式 | Release 模式 | 用途 |
|------|-----|-----------|-------------|------|
| Debug | `MC_ASSERT`, `MC_ASSERT_MSG` | ✅ 启用 | ❌ 禁用 | 开发调试 |
| Release | `MC_ASSERT_RELEASE`, `MC_ASSERT_RELEASE_MSG` | ✅ 启用 | ✅ 启用 | 关键检查 |
| Fatal | `MC_ASSERT_FATAL`, `MC_ASSERT_FATAL_MSG` | ✅ 启用 | ✅ 启用 | 不可恢复错误 |

## 内置处理器参考

| 处理器 | 说明 | 行为 |
|--------|------|------|
| `defaultAssertHandler` | 默认处理器 | 输出到 stderr 并终止 |
| `logAssertHandler` | 日志处理器 | 使用 spdlog 记录并终止 |
| `throwAssertHandler` | 异常处理器 | 抛出 `AssertException` |

## AssertConfig 配置项

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `handler` | `AssertHandler` | `defaultAssertHandler` | 自定义断言处理器 |
| `captureStackTrace` | `bool` | `false` | 是否捕获堆栈跟踪 |
| `breakOnFailure` | `bool` | `false` | 是否触发调试器断点 |
| `throwException` | `bool` | `false` | 是否抛出异常 |
| `continueExecution` | `bool` | `false` | 是否继续执行（仅 Debug 级别） |

## 最佳实践

### 1. 使用描述性消息

```cpp
// ❌ 不够清晰
MC_ASSERT_MSG(ptr, "null");

// ✅ 清晰描述问题
MC_ASSERT_MSG(ptr, "Player pointer must not be null after spawn");
```

### 2. 断言前置条件

```cpp
void processChunk(Chunk* chunk) {
    MC_PRECONDITION(chunk != nullptr);
    MC_PRECONDITION_MSG(chunk->isLoaded(), "Chunk must be loaded before processing");
    // ...
}
```

### 3. 断言类不变量

```cpp
class Buffer {
    size_t m_size, m_capacity;
    char* m_data;

    void validate() const {
        MC_INVARIANT(m_size <= m_capacity);
        MC_INVARIANT(m_data != nullptr || m_capacity == 0);
    }
};
```

### 4. 使用比较断言获得更好的错误信息

```cpp
// ❌ 简单断言看不到值
MC_ASSERT(count == expectedCount);

// ✅ 比较断言显示两个值
MC_ASSERT_EQ(count, expectedCount);
// 输出: count = 5, expectedCount = 10
```

### 5. 在 Release 构建中保持关键检查

```cpp
// 安全关键检查应在所有构建中启用
MC_ASSERT_RELEASE(inputIndex < bufferSize);
MC_ASSERT_FATAL(criticalPointer != nullptr);
```

## 架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                           用户代码                                   │
│                                                                     │
│   MC_ASSERT(ptr != nullptr);                                        │
│   MC_ASSERT_RELEASE(index < size);                                  │
│   MC_ASSERT_EQ(expected, actual);                                   │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      AssertMacros.hpp                               │
│                                                                     │
│   #define MC_ASSERT(cond) MC_ASSERT_IMPL(cond, nullptr, Debug)      │
│   #define MC_ASSERT_RELEASE(cond) MC_ASSERT_IMPL(cond, nullptr, Re) │
│   #define MC_ASSERT_EQ(a, b) ...                                    │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         Assert.hpp                                  │
│                                                                     │
│   ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐    │
│   │  AssertLevel    │  │  AssertFailure  │  │  AssertConfig   │    │
│   │  - Debug        │  │  - expression   │  │  - handler      │    │
│   │  - Release      │  │  - message      │  │  - stackTrace   │    │
│   │  - Fatal        │  │  - file/line    │  │  - breakOnFail  │    │
│   └─────────────────┘  └─────────────────┘  └─────────────────┘    │
│                                                                     │
│   ┌─────────────────────────────────────────────────────────────┐  │
│   │                     AssertManager (单例)                     │  │
│   │                                                             │  │
│   │  - instance()         - handleFailure()                    │  │
│   │  - setConfig()        - handleRecoverableFailure()         │  │
│   │  - setHandler()       - captureStackTrace()                │  │
│   └─────────────────────────────────────────────────────────────┘  │
│                                                                     │
│   ┌─────────────────┐  ┌─────────────────────────────────────┐    │
│   │ AssertException │  │         处理器函数                   │    │
│   │                 │  │  - defaultAssertHandler()           │    │
│   │  - failure()    │  │  - logAssertHandler()              │    │
│   │  - what()       │  │  - throwAssertHandler()            │    │
│   └─────────────────┘  └─────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         Assert.cpp                                  │
│                                                                     │
│   ┌─────────────────────────────────────────────────────────────┐  │
│   │                    跨平台堆栈跟踪                             │  │
│   │                                                             │  │
│   │   Windows: CaptureStackBackTrace + DbgHelp                  │  │
│   │   Linux/macOS: backtrace + abi::__cxa_demangle              │  │
│   └─────────────────────────────────────────────────────────────┘  │
│                                                                     │
│   ┌─────────────────────────────────────────────────────────────┐  │
│   │                    值格式化帮助函数                           │  │
│   │                                                             │  │
│   │   detail::formatValue<T>()        - 通用值格式化            │  │
│   │   detail::formatValue(const char*) - 字符串（带引号）       │  │
│   │   detail::formatValue(bool)       - 布尔值格式化            │  │
│   │   detail::formatValue(T*)         - 指针格式化              │  │
│   │   detail::formatComparisonMessage() - 比较消息格式化        │  │
│   └─────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```
