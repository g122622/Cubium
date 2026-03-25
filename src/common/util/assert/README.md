# Assert Library

一个功能强大的运行时断言库，支持多种断言级别、堆栈跟踪和自定义处理器。

## 快速入门

### 包含头文件

```cpp
#include "common/util/assert/AssertAll.hpp"
```

### 基本用法

```cpp
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

## 断言级别

| 级别 | 宏 | Debug 模式 | Release 模式 | 用途 |
|------|-----|-----------|-------------|------|
| Debug | `MC_ASSERT`, `MC_ASSERT_MSG` | ✅ 启用 | ❌ 禁用 | 开发调试 |
| Release | `MC_ASSERT_RELEASE`, `MC_ASSERT_RELEASE_MSG` | ✅ 启用 | ✅ 启用 | 关键检查 |
| Fatal | `MC_ASSERT_FATAL`, `MC_ASSERT_FATAL_MSG` | ✅ 启用 | ✅ 启用 | 不可恢复错误 |

## 断言宏参考

### 基本断言

```cpp
// 条件断言
MC_ASSERT(condition);                    // Debug 模式
MC_ASSERT_MSG(condition, "message");     // Debug 模式 + 自定义消息

// Release 模式断言
MC_ASSERT_RELEASE(condition);
MC_ASSERT_RELEASE_MSG(condition, "message");

// 致命断言
MC_ASSERT_FATAL(condition);
MC_ASSERT_FATAL_MSG(condition, "message");
```

### 指针断言

```cpp
int* ptr = nullptr;
int value = 42;

MC_ASSERT_NULL(ptr);                 // 检查指针为空（Debug）
MC_ASSERT_NOT_NULL(&value);          // 检查指针非空（Debug）

MC_ASSERT_NULL_RELEASE(ptr);         // Release 模式版本
MC_ASSERT_NOT_NULL_RELEASE(&value);  // Release 模式版本
```

### 范围断言

```cpp
int index = 5;
int size = 10;

// 检查值在闭区间 [min, max] 内
MC_ASSERT_RANGE(index, 0, size - 1);     // 0 <= index <= 9

// 检查索引有效（非负且小于 size）
MC_ASSERT_INDEX(index, size);            // 0 <= index < size

// 检查无符号索引有效
size_t uindex = 5;
MC_ASSERT_INDEX_U(uindex, size);         // uindex < size
```

### 比较断言

比较断言会在失败时输出两个值的实际内容：

```cpp
int expected = 42;
int actual = 100;

MC_ASSERT_EQ(expected, actual);   // 相等断言，失败时显示两个值
MC_ASSERT_NE(a, b);               // 不相等断言
MC_ASSERT_LT(a, b);               // 小于断言 (a < b)
MC_ASSERT_LE(a, b);               // 小于等于断言 (a <= b)
MC_ASSERT_GT(a, b);               // 大于断言 (a > b)
MC_ASSERT_GE(a, b);               // 大于等于断言 (a >= b)
```

失败输出示例：
```
Assertion failed: expected == actual
  not equal
  expected = 42
  actual = 100
```

### 特殊断言

```cpp
// 标记不可达代码路径
void process(int value) {
    switch (value) {
        case 0: /* ... */ break;
        case 1: /* ... */ break;
        default:
            MC_ASSERT_UNREACHABLE();  // 永远不应该到达这里
    }
}

// 带消息的不可达断言
MC_ASSERT_UNREACHABLE_MSG("Unexpected state: " + std::to_string(state));

// 总是失败
MC_ASSERT_FAIL("Critical error: database connection lost");

// 标记未实现功能
MC_ASSERT_NOT_IMPLEMENTED();  // 消息为当前函数名
```

### 前置/后置条件断言

用于函数入口/出口检查：

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
        MC_INVARIANT_MSG(m_count <= m_capacity, "Count exceeds capacity");
    }
};
```

### 调试辅助

```cpp
// 仅在 Debug 模式执行的代码
MC_DEBUG_ONLY(debugLog("Checking state..."));
MC_DEBUG_ONLY(validateInternalState());

// 标记未使用变量（避免编译器警告）
void callback(int value, void* userdata) {
    MC_UNUSED(userdata);  // 我们不需要 userdata
    process(value);
}
```

## 自定义处理器

### 设置自定义处理器

```cpp
#include "common/util/assert/AssertAll.hpp"

void myAssertHandler(const mc::assert::AssertFailure& failure) {
    // 记录日志
    spdlog::error("Assertion failed: {} at {}:{}",
                  failure.expression, failure.file, failure.line);

    // 发送错误报告
    sendErrorReport(failure);

    // 抛出异常或终止
    throw mc::assert::AssertException(failure);
}

int main() {
    mc::assert::AssertConfig config;
    config.handler = myAssertHandler;
    config.captureStackTrace = true;   // 启用堆栈跟踪
    config.breakOnFailure = false;     // 不触发调试器断点

    mc::assert::AssertManager::instance().setConfig(config);

    // ... 应用程序代码 ...
}
```

### 内置处理器

```cpp
// 默认处理器：输出到 stderr 并终止
mc::assert::defaultAssertHandler(failure);

// 日志处理器：使用 spdlog 记录并终止
mc::assert::logAssertHandler(failure);

// 异常处理器：抛出 AssertException
mc::assert::throwAssertHandler(failure);
```

### AssertConfig 配置项

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `handler` | `AssertHandler` | `defaultAssertHandler` | 自定义断言处理器 |
| `captureStackTrace` | `bool` | `false` | 是否捕获堆栈跟踪 |
| `breakOnFailure` | `bool` | `false` | 是否触发调试器断点 |
| `throwException` | `bool` | `false` | 是否抛出异常 |
| `continueExecution` | `bool` | `false` | 是否继续执行（仅 Debug 级别） |

## AssertException

当使用 `throwAssertHandler` 或自定义处理器抛出异常时：

```cpp
try {
    MC_ASSERT_RELEASE(ptr != nullptr);
} catch (const mc::assert::AssertException& e) {
    const auto& failure = e.failure();

    std::cout << "Expression: " << failure.expression << "\n";
    std::cout << "Message: " << failure.message << "\n";
    std::cout << "File: " << failure.file << ":" << failure.line << "\n";
    std::cout << "Function: " << failure.function << "\n";
    std::cout << "Stack trace:\n" << failure.stackTrace << "\n";

    std::cout << "What: " << e.what() << "\n";  // 完整的错误信息
}
```

## 注意事项

### 1. 断言级别选择

- **Debug 级别**：用于开发和调试，不应依赖其副作用
  ```cpp
  // ❌ 错误：不要依赖断言的副作用
  MC_ASSERT(initialize());  // initialize() 在 Release 中不会被调用！

  // ✅ 正确：先执行，再断言
  bool ok = initialize();
  MC_ASSERT(ok);
  ```

- **Release 级别**：用于关键检查，始终执行
  ```cpp
  // ✅ 适合用于输入验证、边界检查
  MC_ASSERT_RELEASE(index < buffer.size());
  ```

- **Fatal 级别**：用于不可恢复的错误
  ```cpp
  // ✅ 适合用于程序状态严重错误
  MC_ASSERT_FATAL(memoryPool != nullptr);  // 没有内存池无法继续
  ```

### 2. 性能考虑

- 通过的断言开销极小（仅一个条件判断）
- 在热点代码路径中使用 Release 级别断言
- 捕获堆栈跟踪有性能开销，仅用于调试

### 3. 不要在断言中执行重要操作

```cpp
// ❌ 错误
MC_ASSERT(saveToFile(data));  // saveToFile 在 Release 中不执行

// ✅ 正确
bool saved = saveToFile(data);
MC_ASSERT_RELEASE(saved);
```

### 4. 断言 vs 异常

- **断言**：用于检查程序内部逻辑错误（不可恢复）
- **异常**：用于处理可预期的错误条件（可恢复）

```cpp
// 断言：检查内部不变量
MC_ASSERT(m_count >= 0);  // 这是程序错误

// 异常：处理外部输入
if (userInput < 0) {
    throw std::invalid_argument("Input must be positive");  // 这是用户错误
}
```

### 5. 比较断言的类型要求

比较断言要求操作数支持对应的运算符：

```cpp
MC_ASSERT_EQ(a, b);  // 要求 a == b
MC_ASSERT_LT(a, b);  // 要求 a < b

// 自定义类型需要定义运算符
struct Vec2 {
    int x, y;
    bool operator==(const Vec2& other) const {
        return x == other.x && y == other.y;
    }
};
```

### 6. 多线程环境

断言管理器是单例，在多线程环境中：

- 处理器应该是线程安全的
- 避免在断言处理器中修改共享状态

```cpp
// 线程安全的处理器示例
std::mutex g_logMutex;
void threadSafeHandler(const mc::assert::AssertFailure& failure) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    logToFile(failure);
}
```

## 架构

```
util/assert/
├── Assert.hpp           # 核心类和接口
│   ├── AssertLevel      # 断言级别枚举
│   ├── AssertFailure    # 断言失败信息
│   ├── AssertConfig     # 配置结构
│   ├── AssertManager    # 单例管理器
│   └── AssertException  # 异常类
├── AssertMacros.hpp     # 断言宏定义
│   ├── MC_ASSERT_*      # 基本断言
│   ├── MC_ASSERT_EQ_*   # 比较断言
│   ├── MC_ASSERT_NULL_* # 指针断言
│   └── MC_ASSERT_*_MSG  # 带消息断言
├── Assert.cpp           # 实现文件
└── AssertAll.hpp        # 统一包含头文件
```

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

public:
    void push(char c) {
        validate();  // 前置检查
        // ... 操作 ...
        validate();  // 后置检查
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
