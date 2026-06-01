#include "common/mod/bedrock/addon/plugin/ScriptPackConfiguration.hpp"

#include <algorithm>

namespace mc::mod::bedrock::addon {

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
