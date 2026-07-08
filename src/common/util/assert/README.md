# Assert Library

运行时断言库，支持多种断言级别、堆栈跟踪和自定义处理器。

## 目录结构树

```
src/common/util/assert/
├── Assert.hpp          # 核心类和接口定义（AssertLevel、AssertFailure、AssertManager、AssertException）
├── Assert.cpp          # 实现文件（跨平台堆栈跟踪、处理器实现）
├── AssertMacros.hpp    # 断言宏定义（MC_ASSERT_* 系列）
├── AssertAll.hpp       # 统一包含入口
├── CrashHandler.hpp    # 崩溃处理器接口（SEH/信号/terminate 捕获）
├── CrashHandler.cpp    # 崩溃处理器实现（调用栈+局部变量输出）
└── README.md           # 本文档
```

## 内部模块关系

```
AssertAll.hpp（用户入口）
    ├── Assert.hpp（核心类型定义）
    ├── AssertMacros.hpp（断言宏，依赖 Assert.hpp）
    └── CrashHandler.hpp（崩溃处理器，独立于 Assert.hpp）

Assert.hpp ←→ Assert.cpp（实现关系）
AssertMacros.hpp → AssertManager::handleFailure()（宏调用管理器）
CrashHandler.hpp ←→ CrashHandler.cpp（崩溃处理器实现，依赖 DbgHelp/信号处理）
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

| 依赖 | 说明 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义（`i32`, `u8`, `std::string` 等） |
| `<exception>` | 异常基类 |
| `<functional>` | `std::function` |
| `<sstream>` | 字符串流 |
| `<vector>` | 动态数组 |

平台特定依赖：
- **Windows**: `Windows.h`, `DbgHelp.h`, `dbghelp.lib`（堆栈跟踪）
- **Linux/macOS**: `execinfo.h`, `cxxabi.h`（堆栈跟踪）

### 下游依赖（依赖本模块的）

全项目通用基础设施，几乎所有模块都通过 `AssertAll.hpp` 使用断言功能。

## 容易踩的坑

### 1. 【重要】项目只允许使用部分断言宏

虽然 `AssertAll.hpp` 提供了大量断言工具，但根据项目规范，**目前只允许使用**：
- `MC_ASSERT_RELEASE(cond)` - Release 模式断言（始终启用）
- `MC_ASSERT_RELEASE_MSG(cond, msg)` - 带消息的 Release 断言
- `MC_UNUSED(var)` - 未使用变量标记

不允许使用 `MC_ASSERT`、`MC_ASSERT_FATAL`、`MC_ASSERT_EQ` 等其他断言宏。

### 2. 断言中的副作用

Debug 断言在 Release 模式下被禁用，其中的代码不会执行：

```cpp
// ❌ 错误：initialize() 在 Release 中不会被调用
MC_ASSERT(initialize());

// ✅ 正确：先执行，再断言
bool ok = initialize();
MC_ASSERT_RELEASE(ok);
```

### 3. Release 构建中的性能开销

即使在 Release 构建，`MC_ASSERT_RELEASE` 也会有轻微开销。热点代码路径中谨慎使用：

```cpp
for (int i = 0; i < millions; ++i) {
    // ❌ 可能影响性能
    MC_ASSERT_RELEASE(data[i] != nullptr);

    // ✅ 更高效的方式
    if (data[i] == nullptr) [[unlikely]] {
        handleNullCase();
    }
}
```

### 4. 多线程环境

断言管理器是单例，默认处理器 `defaultAssertHandler()` 使用全局互斥锁串行化输出，避免多线程同时断言时控制台输出互相穿插。自定义处理器必须是线程安全的。

### 5. 断言 vs 异常

- **断言**：用于检查程序内部逻辑错误（不可恢复）
- **异常**：用于处理可预期的错误条件（可恢复）

### 6. 堆栈跟踪性能开销

断言失败默认捕获调用栈（`AssertConfig::captureStackTrace = true`），与 `CrashHandler` 的崩溃栈输出对齐，便于在没有调试器附加时定位失败根因。仅在断言**失败**时触发，不影响正常路径性能。如需全局禁用（例如压测热路径），设置环境变量 `MC_ASSERT_NO_STACK=1` 即可。

### 7. 比较断言的类型要求

比较断言（如 `MC_ASSERT_EQ`）要求操作数支持对应的运算符（`==`, `<` 等），自定义类型需要定义相应运算符。

### 8. CrashHandler 局部变量枚举

`CrashHandler` 在 Windows 上尝试枚举每个栈帧的局部变量（使用 `SymSetContext` + `SymEnumSymbols`），但 RelWithDebInfo 下编译器优化会内联/消除局部变量，**输出可能不完整**。如果需要完整的变量信息，请使用 Debug 构建或在崩溃后用调试器附加。

### 9. CrashHandler 与 MC_ASSERT 的关系

`CrashHandler` 捕获的是**未处理的崩溃**（SEH 异常、信号、纯虚函数调用、std::terminate 等）；`MC_ASSERT_*` 触发的断言走 `AssertManager` 的处理流程。两者栈输出机制现已统一：`AssertManager::captureStackTrace()` 委托 `CrashHandler::captureStackTrace()`，断言失败时由 `defaultAssertHandler` 经 `formatFailureBlock` 一并输出调用栈，随后 `std::abort()` 触发 `CrashHandler` 的 `SIGABRT`/SEH 路径（栈可能被二次输出，属正常现象）。
