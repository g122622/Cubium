#pragma once

namespace mc::test {

/**
 * @brief 测试函数上下文接口。
 *
 * 对齐基岩版 `IGameTestFunctionContext`：测试函数执行时携带的上下文对象。原生 C++ 测试通常用空实现
 *（`EmptyGameTestFunctionContext`），脚本测试用 `ScriptGameTestFunctionContext` 持有 `ScriptGameTestHelper`
 * 句柄。`BaseGameTestFunction::createContext(helper)` 由子类实现，构造对应上下文。
 *
 * 此接口仅有虚析构，具体字段由实现子类定义。框架核心通过 `unique_ptr<IGameTestFunctionContext>`
 * 持有，不依赖具体类型。
 */
class IGameTestFunctionContext {
public:
    virtual ~IGameTestFunctionContext() = default;
};

/**
 * @brief 空的测试函数上下文（原生 C++ 测试用）。
 *
 * 对齐基岩版 `EmptyGameTestFunctionContext`。原生测试函数签名 `void(GameTestHelper&)` 不需额外上下文，
 * 故用此空实现。
 */
class EmptyGameTestFunctionContext final : public IGameTestFunctionContext {
public:
    EmptyGameTestFunctionContext() = default;
};

} // namespace mc::test
