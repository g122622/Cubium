#include "common/test/framework/registry/GameTestRegistry.hpp"

#include "common/test/framework/function/BaseGameTestFunction.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc::test {

GameTestRegistry& GameTestRegistry::instance() noexcept
{
    static GameTestRegistry s_instance;
    return s_instance;
}

bool GameTestRegistry::registerTestMethod(const std::string& className, std::shared_ptr<BaseGameTestFunction> fn)
{
    if (!fn) {
        return false;
    }
    const std::string testName = fn->testName();
    if (m_byName.find(testName) != m_byName.end()) {
        return false; // 同名已存在
    }
    spdlog::info("[GameTest] Registered test '{}.{}' (structure={})", className, testName, fn->structureName());
    m_byName[testName] = fn;
    m_byClass[className].push_back(std::move(fn));
    return true;
}

std::shared_ptr<BaseGameTestFunction> GameTestRegistry::getTestFunction(const std::string& testName) const
{
    const auto it = m_byName.find(testName);
    return it != m_byName.end() ? it->second : nullptr;
}

std::vector<std::shared_ptr<BaseGameTestFunction>> GameTestRegistry::allTestFunctions() const
{
    std::vector<std::shared_ptr<BaseGameTestFunction>> all;
    all.reserve(m_byName.size());
    for (const auto& [name, fn] : m_byName) {
        all.push_back(fn);
    }
    // m_byName 是 unordered_map，迭代顺序运行间不固定，会致下游 BatchRunner 的结构原点分配
    // （m_nextOrigin 游标按测试顺序推进）非确定，进而使依赖精确方块/坐标的测试 flaky
    // （如 sheep_eat_grass 脚下方块在 grass/cobblestone 间漂移）。按 testName 字典序排序，
    // 保证测试执行顺序与结构原点分配在运行间确定。
    std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) { return a->testName() < b->testName(); });
    return all;
}

std::vector<std::shared_ptr<BaseGameTestFunction>> GameTestRegistry::getTestsByPattern(const std::string& pattern) const
{
    // TODO: 完整通配符匹配（* / ?）；当前支持 "<prefix>.*"（按 testName 前缀）与全等 testName。
    // 对齐 Java GameTestMainUtil --tests：模式按测试名（testName）匹配，非 className。
    std::vector<std::shared_ptr<BaseGameTestFunction>> result;
    if (pattern.empty()) {
        return allTestFunctions();
    }
    const auto dot = pattern.find(".*");
    if (dot != std::string::npos) {
        // 前缀匹配 testName（如 "pat.*" 命中 "pat_one"/"pat_two"）
        const std::string prefix = pattern.substr(0, dot);
        for (const auto& [name, fn] : m_byName) {
            if (name.size() >= prefix.size() && name.compare(0, prefix.size(), prefix) == 0) {
                result.push_back(fn);
            }
        }
    } else {
        // 全等 testName
        const auto it = m_byName.find(pattern);
        if (it != m_byName.end()) {
            result.push_back(it->second);
        }
    }
    // 同 allTestFunctions：排序保证下游结构原点分配确定（见其注释）。
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a->testName() < b->testName(); });
    return result;
}

std::vector<std::shared_ptr<BaseGameTestFunction>> GameTestRegistry::getTestsByTag(const std::string& tag) const
{
    // TODO: 标签索引优化；当前线性扫描
    std::vector<std::shared_ptr<BaseGameTestFunction>> result;
    for (const auto& [name, fn] : m_byName) {
        if (fn->hasTag(tag)) {
            result.push_back(fn);
        }
    }
    // 同 allTestFunctions：排序保证下游结构原点分配确定（见其注释）。
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a->testName() < b->testName(); });
    return result;
}

bool GameTestRegistry::registerBeforeBatchFunction(const std::string& batchName, std::function<void()> fn)
{
    m_beforeBatch[batchName] = std::move(fn);
    return true;
}

std::function<void()> GameTestRegistry::getBeforeBatchFunction(const std::string& batchName) const
{
    const auto it = m_beforeBatch.find(batchName);
    return it != m_beforeBatch.end() ? it->second : std::function<void()>{};
}

bool GameTestRegistry::registerAfterBatchFunction(const std::string& batchName, std::function<void()> fn)
{
    m_afterBatch[batchName] = std::move(fn);
    return true;
}

std::function<void()> GameTestRegistry::getAfterBatchFunction(const std::string& batchName) const
{
    const auto it = m_afterBatch.find(batchName);
    return it != m_afterBatch.end() ? it->second : std::function<void()>{};
}

void GameTestRegistry::clearAllTestMethods()
{
    m_byClass.clear();
    m_byName.clear();
    m_beforeBatch.clear();
    m_afterBatch.clear();
}

void GameTestRegistry::releaseAllScriptResources()
{
    // 遍历所有测试函数释放 JS 句柄。m_byName 与 m_byClass 指向同一批 shared_ptr，遍历其一即可。
    // 须在脚本引擎销毁前调用；调用后 function 对象仍存活（仅 JS 句柄置空），析构安全。
    for (const auto& [name, fn] : m_byName) {
        if (fn != nullptr) {
            fn->releaseScriptResources();
        }
    }
}

} // namespace mc::test
