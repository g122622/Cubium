/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "LootParameterSet.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace mc {
namespace loot {

// ============================================================================
// LootParameterSet
// ============================================================================

bool LootParameterSet::contains(std::string_view paramId) const noexcept
{
    // 在必需参数中查找
    auto requiredIt = std::find_if(
        m_requiredParams.begin(), m_requiredParams.end(), [&paramId](const std::string& id) { return id == paramId; });
    if (requiredIt != m_requiredParams.end()) {
        return true;
    }

    // 在可选参数中查找
    auto optionalIt = std::find_if(
        m_optionalParams.begin(), m_optionalParams.end(), [&paramId](const std::string& id) { return id == paramId; });
    return optionalIt != m_optionalParams.end();
}

bool LootParameterSet::validate(const std::vector<std::string>& providedParams) const noexcept
{
    std::vector<std::string> missingParams;
    std::vector<std::string> unexpectedParams;
    return validate(providedParams, missingParams, unexpectedParams);
}

bool LootParameterSet::validate(const std::vector<std::string>& providedParams,
    std::vector<std::string>& missingParams,
    std::vector<std::string>& unexpectedParams) const noexcept
{
    missingParams.clear();
    unexpectedParams.clear();

    // 检查1：提供的参数中是否包含此参数集不允许的参数
    for (const auto& provided : providedParams) {
        if (!isAllowed(provided)) {
            unexpectedParams.push_back(provided);
        }
    }

    // 检查2：是否缺少此参数集必需的参数
    for (const auto& required : m_requiredParams) {
        bool found = std::any_of(providedParams.begin(),
            providedParams.end(),
            [&required](const std::string& provided) { return required == provided; });
        if (!found) {
            missingParams.push_back(required);
        }
    }

    return missingParams.empty() && unexpectedParams.empty();
}

bool LootParameterSet::isAllowed(std::string_view paramId) const noexcept
{
    // 在必需参数中查找
    auto requiredIt = std::find_if(
        m_requiredParams.begin(), m_requiredParams.end(), [&paramId](const std::string& id) { return id == paramId; });
    if (requiredIt != m_requiredParams.end()) {
        return true;
    }

    // 在可选参数中查找
    auto optionalIt = std::find_if(
        m_optionalParams.begin(), m_optionalParams.end(), [&paramId](const std::string& id) { return id == paramId; });
    return optionalIt != m_optionalParams.end();
}

} // namespace loot
} // namespace mc
