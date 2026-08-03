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

#include "common/mod/bedrock/addon/plugin/ScriptPackConfiguration.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPackPermissions.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace mc::mod::bedrock::addon {

ScriptPackConfiguration::ScriptPackConfiguration(ScriptPackConfiguration&& other) noexcept
    : m_allowedModules(std::move(other.m_allowedModules))
    , m_excludedModules(std::move(other.m_excludedModules))
    , m_permissions(std::move(other.m_permissions))
{}

ScriptPackConfiguration& ScriptPackConfiguration::operator=(ScriptPackConfiguration&& other) noexcept
{
    if (this != &other) {
        m_allowedModules = std::move(other.m_allowedModules);
        m_excludedModules = std::move(other.m_excludedModules);
        m_permissions = std::move(other.m_permissions);
    }
    return *this;
}

void ScriptPackConfiguration::setAllowedModules(std::vector<std::string> modules)
{
    m_allowedModules = std::move(modules);
}

void ScriptPackConfiguration::setExcludedModules(std::vector<std::string> modules)
{
    m_excludedModules = std::move(modules);
}

void ScriptPackConfiguration::setPermissions(ScriptPackPermissions permissions)
{
    m_permissions = std::move(permissions);
}

bool ScriptPackConfiguration::isModuleAllowed(const std::string& moduleName) const
{
    // 排除列表优先：被排除的模块始终不允许
    if (isModuleExcluded(moduleName)) {
        return false;
    }
    // 允许列表为空时允许所有模块
    if (m_allowedModules.empty()) {
        return true;
    }
    return std::find(m_allowedModules.begin(), m_allowedModules.end(), moduleName) != m_allowedModules.end();
}

bool ScriptPackConfiguration::isModuleExcluded(const std::string& moduleName) const
{
    return std::find(m_excludedModules.begin(), m_excludedModules.end(), moduleName) != m_excludedModules.end();
}

const ScriptPackPermissions& ScriptPackConfiguration::permissions() const
{
    return m_permissions;
}

} // namespace mc::mod::bedrock::addon
