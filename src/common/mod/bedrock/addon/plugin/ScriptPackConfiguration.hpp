#pragma once

#include "common/mod/bedrock/addon/plugin/ScriptPackPermissions.hpp"

#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本包配置
 *
 * 解析自行为包manifest.json，控制脚本模块的加载行为。
 * 包含模块名称、允许/排除列表和权限。
 */
class ScriptPackConfiguration {
public:
    ScriptPackConfiguration() = default;

    /**
     * @brief 设置允许的模块列表
     * @param modules 模块名称列表（如"@minecraft/server"）
     */
    void setAllowedModules(std::vector<std::string> modules);

    /**
     * @brief 设置排除的模块列表
     * @param modules 模块名称列表
     */
    void setExcludedModules(std::vector<std::string> modules);

    /**
     * @brief 设置脚本权限
     */
    void setPermissions(ScriptPackPermissions permissions);

    /**
     * @brief 检查指定模块是否被允许加载
     * @param moduleName 模块名称
     * @return 如果允许列表为空则全部允许，否则检查是否在列表中
     */
    [[nodiscard]] bool isModuleAllowed(const std::string& moduleName) const;

    /**
     * @brief 检查指定模块是否被排除
     * @param moduleName 模块名称
     */
    [[nodiscard]] bool isModuleExcluded(const std::string& moduleName) const;

    /**
     * @brief 获取权限
     */
    [[nodiscard]] const ScriptPackPermissions& permissions() const;

private:
    std::vector<std::string> m_allowedModules;
    std::vector<std::string> m_excludedModules;
    ScriptPackPermissions m_permissions;
};

} // namespace mc::mod::bedrock::addon
