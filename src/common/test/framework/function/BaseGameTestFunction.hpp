#pragma once

#include "common/test/base/data/TestData.hpp"
#include "common/test/framework/function/IGameTestFunctionContext.hpp"
#include "common/test/framework/function/IGameTestRunResult.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mc::test {

// 前向声明：避免 function/ 与 helper/ 互引成环。IGameTestHelper 在 framework/helper/ 定义。
class IGameTestHelper;

/**
 * @brief 测试函数抽象基类。
 *
 * 对齐基岩版 `BaseGameTestFunction`：持测试元数据（`TestData`）+ 批次名/标签，并提供两个纯虚工厂方法：
 * - `createContext(helper)`：构造测试函数执行上下文（原生用 `EmptyGameTestFunctionContext`，脚本用
 *   `ScriptGameTestFunctionContext` 持 helper 句柄）。
 * - `run(helper, ctx)`：执行测试函数，返回 `IGameTestFunctionRunResult`（同步立即完成 / 异步轮询）。
 *
 * 子类：
 * - `NativeGameTestFunction`（`native/`）：持 `std::function<GameTestResult(GameTestHelper&)>`。
 * - `BaseScriptGameTestFunction`（`script/`）：持 JS Closure。
 *
 * 元数据字段（`TestData`）合并了 Java `TestData`（10 字段）+ 基岩 `padding`/`batchName`。
 * `hasTag` 用于运行期按标签筛选。
 */
class BaseGameTestFunction {
public:
    BaseGameTestFunction(std::string batchName, std::string testName, std::string structureName, TestData data)
        : m_batchName(std::move(batchName))
        , m_testName(std::move(testName))
        , m_structureName(std::move(structureName))
        , m_data(std::move(data))
    {}

    virtual ~BaseGameTestFunction() = default;

    /**
     * @brief 构造测试函数执行上下文。
     */
    [[nodiscard]] virtual std::unique_ptr<IGameTestFunctionContext> createContext(IGameTestHelper& helper) const = 0;

    /**
     * @brief 执行测试函数。
     *
     * @param helper 测试助手（门面 `GameTestHelper` 的内部接口）。
     * @param context 由 `createContext` 构造的上下文。
     * @return 运行结果轮询句柄（同步/异步）。
     */
    [[nodiscard]] virtual std::unique_ptr<IGameTestFunctionRunResult> run(
        IGameTestHelper& helper, IGameTestFunctionContext& context) const = 0;

    [[nodiscard]] const std::string& batchName() const noexcept { return m_batchName; }
    [[nodiscard]] const std::string& testName() const noexcept { return m_testName; }
    [[nodiscard]] const std::string& structureName() const noexcept { return m_structureName; }
    [[nodiscard]] const TestData& data() const noexcept { return m_data; }
    [[nodiscard]] const std::vector<std::string>& tags() const noexcept { return m_tags; }

    void addTag(std::string tag) { m_tags.push_back(std::move(tag)); }
    [[nodiscard]] bool hasTag(std::string_view tag) const noexcept;

    /**
     * @brief 释放脚本资源（JS 句柄等）。
     *
     * 脚本测试函数（`ScriptGameTestFunction`）持有 JS 回调句柄，其生命周期绑脚本引擎 runtime。
     * 引擎销毁前须调用此方法释放句柄，否则 registry 单例在进程退出/atexit 析构 function 时，
     * 会对已死 runtime 的 JSContext 调 JS_FreeValue → use-after-free 崩溃。
     * 原生测试函数无 JS 资源，默认空实现。
     * 调用后 function 对象仍可安全析构（子类须将句柄置空，析构不再 release）。
     */
    virtual void releaseScriptResources() {}

protected:
    // 子类（NativeGameTestFunction/ScriptGameTestFunction）可改写元数据
    void setTestData(TestData data) { m_data = std::move(data); }

private:
    std::string m_batchName;
    std::string m_testName;
    std::string m_structureName;
    TestData m_data;
    std::vector<std::string> m_tags;
};

} // namespace mc::test
