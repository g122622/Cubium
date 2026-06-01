#pragma once

#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"
#include <string>

namespace mc::mod::bedrock::addon {

/**
 * @brief 模块依赖声明
 *
 * 描述一个模块对另一个模块的依赖关系
 */
struct ModuleDependency {
    std::string name;            // 依赖模块名，如 "@minecraft/server"
    ModuleVersion version;       // 最低版本要求
    std::string preRelease;      // 预发布标识，如 "beta"

    [[nodiscard]] bool isNativeDependency() const {
        // 以 @minecraft/ 开头的是原生模块依赖
        return name.starts_with("@minecraft/");
    }

    [[nodiscard]] bool isPackDependency() const {
        // UUID 格式的是包间依赖
        return !name.empty() && name.size() == 36 && name[8] == '-' && name[13] == '-' &&
               name[18] == '-' && name[23] == '-';
    }
};

} // namespace mc::mod::bedrock::addon
