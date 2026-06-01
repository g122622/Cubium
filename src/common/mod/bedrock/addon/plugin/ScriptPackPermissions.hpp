#pragma once

#include "common/core/Types.hpp"

#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本包权限
 *
 * 定义行为包脚本允许执行的操作。
 * 权限在manifest.json的capabilities字段中声明。
 */
enum class ScriptPermission : u32 {
    /// 允许使用eval()
    AllowEval = 1 << 0,
};

/**
 * @brief 脚本包权限集合
 *
 * 基于位掩码的权限管理，解析自manifest的capabilities字段。
 */
class ScriptPackPermissions {
public:
    ScriptPackPermissions() = default;

    /**
     * @brief 从capabilities字符串列表构造权限
     * @param capabilities manifest中声明的capabilities
     */
    explicit ScriptPackPermissions(const std::vector<std::string>& capabilities);

    /**
     * @brief 设置权限
     */
    void setPermission(ScriptPermission perm, bool enabled = true);

    /**
     * @brief 检查是否拥有指定权限
     */
    [[nodiscard]] bool hasPermission(ScriptPermission perm) const;

    /**
     * @brief 获取原始权限位掩码
     */
    [[nodiscard]] u32 rawFlags() const { return m_flags; }

private:
    u32 m_flags = 0;
};

} // namespace mc::mod::bedrock::addon
