#pragma once

#include "common/core/Types.hpp"

namespace mc::mod::bedrock::addon {

/**
 * @brief 插件执行分组
 *
 * 控制插件脚本在服务器生命周期的哪个阶段执行。
 * 分组决定了脚本的初始化时机和可用API范围。
 */
enum class PluginExecutionGroup : u8 {
    /// 在包加载之前执行（早期初始化，有限API）
    PrePackLoad = 0,
    /// 服务器启动时执行（标准时机，完整API）
    ServerStart = 1,
    /// 客户端等级加载时执行（世界级脚本）
    ClientLevel = 2,
};

/**
 * @brief 获取执行分组的显示名称
 */
[[nodiscard]] const char* pluginExecutionGroupName(PluginExecutionGroup group);

} // namespace mc::mod::bedrock::addon
