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

// 通配符匹配，对齐 Java --tests 用的 FilenameUtils.wildcardMatch 语义：
//   *  匹配任意长度序列（含空）
//   ?  匹配单个字符
//   其余字符字面匹配
//   大小写敏感（Java 默认 IOCase.SENSITIVE，与 ResourceSelectorArgument.matches 一致）
// 采用迭代回溯（双指针 + star 回溯），O(n*m) 最坏、O(n) 常态，无递归栈开销。
// 实现 ResourceSelectorArgument.java:73 matches → FilenameUtils.wildcardMatch。
static bool wildcardMatch(std::string_view text, std::string_view pattern)
{
    size_t ti = 0;                        // text 游标
    size_t pi = 0;                        // pattern 游标
    size_t star = std::string_view::npos; // 最近一个 '*' 的位置
    size_t match = 0;                     // 与该 '*' 对齐的 text 位置（回溯点）

    while (ti < text.size()) {
        if (pi < pattern.size() && (pattern[pi] == '?' || pattern[pi] == text[ti])) {
            // 单字符匹配或 '?' 通配：双游标前进
            ++ti;
            ++pi;
        } else if (pi < pattern.size() && pattern[pi] == '*') {
            // 记录 '*' 位置，先尝试让 '*' 匹配空串（match 不前进）
            star = pi;
            match = ti;
            ++pi;
        } else if (star != std::string_view::npos) {
            // 当前不匹配但存在回溯点：让上一个 '*' 多吃一个 text 字符后重试
            pi = star + 1;
            ++match;
            ti = match;
        } else {
            // 无通配符可回溯，匹配失败
            return false;
        }
    }

    // text 已耗尽，pattern 剩余必须全是 '*' 才算匹配
    while (pi < pattern.size() && pattern[pi] == '*') {
        ++pi;
    }
    return pi == pattern.size();
}

std::vector<std::shared_ptr<BaseGameTestFunction>> GameTestRegistry::getTestsByPattern(const std::string& pattern) const
{
    // 对齐 Java --tests（GameTestMainUtil → GameTestServer.getTestsForSelection →
    // ResourceSelectorArgument → FilenameUtils.wildcardMatch）：按 testName 做 * / ? 通配符匹配。
    // Cubium 无 namespace 概念，testName 即匹配目标（Java 是 namespace:testName 全串匹配，
    // 此处等价于对 testName 单段匹配）。
    //   - "*" / 空串 → 全部
    //   - "pat*" → 命中 pat_one / pat_two
    //   - "*llama*" → 命中含 llama 子串的 testName（修复旧 prefix.* 实现下 .*llama.* 因
    //     空前缀静默匹配全部的 bug）
    //   - "exact_name" → 全等
    std::vector<std::shared_ptr<BaseGameTestFunction>> result;
    if (pattern.empty() || pattern == "*") {
        return allTestFunctions();
    }
    for (const auto& [name, fn] : m_byName) {
        if (wildcardMatch(name, pattern)) {
            result.push_back(fn);
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
