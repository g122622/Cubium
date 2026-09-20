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
     * @brief 按 testName 通配符筛选测试，对齐 Java `--tests` 语义。
     *
     * 按 testName（非 className）做 `*` / `?` 通配符匹配，等价 Java
     * `ResourceSelectorArgument` → `FilenameUtils.wildcardMatch`：
     *   - `*` 匹配任意长度序列（含空）
     *   - `?` 匹配单个字符
     *   - 其余字符字面匹配，大小写敏感
     *   - 空串或 `"*"` 返回全部
     *
     * 示例：`"pat*"` 命中 `pat_one`/`pat_two`；`"*llama*"` 命中含 `llama` 子串的 testName。
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
     * @brief 释放所有脚本测试函数的 JS 回调句柄，并移除其注册记录
     *
     * 脚本引擎销毁前调用：对每个 `isScriptBacked()` 为真的函数先调 `releaseScriptResources()`
     * 释放 JS 回调句柄，再把该条目从 `m_byName` / `m_byClass` 中移除。
     *
     * 必须"先释放后移除"：移除使引用计数归零，function 对象随即析构；句柄已置空，析构不再对
     * 已死/将死 runtime 的 JSContext 调 JS_FreeValue（否则即 use-after-free 崩溃）。
     *
     * 之所以要移除条目：句柄绑定在**当前**引擎 runtime 上，释放后该函数永久不可运行。若保留记录，
     * 同一进程内下一个 `GameTestServer`（新引擎）重新加载行为包时同名注册会被 `registerTestMethod`
     * 的同名判据拒绝，留下的仍是失效旧条目（表现为 `Script test function has no JS callback`）。
     *
     * 原生测试函数（不依赖 JS runtime）不受影响，保留在注册表中跨实例复用。
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
