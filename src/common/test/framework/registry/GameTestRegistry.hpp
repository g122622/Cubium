#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::test {

class BaseGameTestFunction;

/**
 * @brief 测试注册表（数据容器）。
 *
 * 对齐基岩版 `GameTestRegistry`：存储所有已注册的测试函数（按 className/testName 索引）+ 批次回调
 *（before/after）。这是内部数据容器，**不对外**——外部经门面 `GameTestRegistrar` 间接访问。
 *
 * 原生测试（`NativeGameTestFunction`）与脚本测试（`BaseScriptGameTestFunction`）均汇入此注册表，
 * 是原生与脚本的汇聚点。
 *
 * 单例（`instance()`），整个进程共享。`GameTestRegistrar::register` 经此注册；
 * `GameTestServer`/`GameTestCommand` 经此查询要运行的测试。
 */
class GameTestRegistry {
public:
    [[nodiscard]] static GameTestRegistry& instance() noexcept;

    /**
     * @brief 注册测试方法。
     *
     * @param className 组织/套件名（如 `"ExampleTests"`，对应 JS `register(testClassName,...)` 首参）。
     * @param fn 测试函数（`std::shared_ptr<BaseGameTestFunction>`）。
     * @return true=注册成功，false=同名已存在。
     */
    bool registerTestMethod(const std::string& className, std::shared_ptr<BaseGameTestFunction> fn);

    /**
     * @brief 按 testName 查询测试函数。
     */
    [[nodiscard]] std::shared_ptr<BaseGameTestFunction> getTestFunction(const std::string& testName) const;

    /**
     * @brief 取所有已注册测试函数。
     */
    [[nodiscard]] std::vector<std::shared_ptr<BaseGameTestFunction>> allTestFunctions() const;

    /**
     * @brief 按 className 前缀筛选（`--tests` 通配符的简化版，支持 `"Suite.*"` 匹配）。
     *
     * TODO: 完整通配符（`*`/`?`）匹配待实现，当前仅支持 `"<prefix>.*"` 与全等。
     */
    [[nodiscard]] std::vector<std::shared_ptr<BaseGameTestFunction>> getTestsByPattern(
        const std::string& pattern) const;

    /**
     * @brief 按标签筛选测试。
     *
     * TODO: 标签索引待实现（当前线性扫描）。
     */
    [[nodiscard]] std::vector<std::shared_ptr<BaseGameTestFunction>> getTestsByTag(const std::string& tag) const;

    /**
     * @brief 注册批次前置回调。
     */
    bool registerBeforeBatchFunction(const std::string& batchName, std::function<void()> fn);
    [[nodiscard]] std::function<void()> getBeforeBatchFunction(const std::string& batchName) const;

    /**
     * @brief 注册批次后置回调。
     */
    bool registerAfterBatchFunction(const std::string& batchName, std::function<void()> fn);
    [[nodiscard]] std::function<void()> getAfterBatchFunction(const std::string& batchName) const;

    /**
     * @brief 清空所有注册（用于重启/测试隔离）。
     */
    void clearAllTestMethods();

    /**
     * @brief 释放所有测试函数的脚本资源（JS 句柄）。
     *
     * 脚本引擎销毁前调用：遍历所有 `BaseGameTestFunction` 调 `releaseScriptResources()`，
     * 释放 `ScriptGameTestFunction` 持有的 JS 回调句柄。不删除 function 对象（registry 仍持有，
     * 供后续查询/跨用例），仅清 JS 句柄——避免引擎销毁后 registry 单例析构 function 时
     * 对已死 JSContext 调 JS_FreeValue 崩溃。
     */
    void releaseAllScriptResources();

private:
    GameTestRegistry() = default;

    // className → 测试函数列表；同时 testName → 函数 的扁平索引便于按名查询
    std::unordered_map<std::string, std::vector<std::shared_ptr<BaseGameTestFunction>>> m_byClass;
    std::unordered_map<std::string, std::shared_ptr<BaseGameTestFunction>> m_byName;
    std::unordered_map<std::string, std::function<void()>> m_beforeBatch;
    std::unordered_map<std::string, std::function<void()>> m_afterBatch;
};

} // namespace mc::test
