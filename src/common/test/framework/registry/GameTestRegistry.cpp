#include "common/test/framework/registry/GameTestRegistry.hpp"

#include "common/test/framework/function/BaseGameTestFunction.hpp"

#include <algorithm>

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

} // namespace mc::test
