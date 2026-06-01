#pragma once

#include "common/core/Types.hpp"

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本执行权限级别
 */
enum class Privilege : u8 {
    Default = 0,               // 默认权限
    RestrictedExecAllowed = 1, // 允许受限执行
    EarlyExecAllowed = 2,     // 允许早期执行（PrePackLoad阶段）
};

} // namespace mc::mod::bedrock::addon
