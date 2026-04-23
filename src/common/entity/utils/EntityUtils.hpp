#pragma once

#include "../core/Entity.hpp"

namespace mc {

/**
 * @brief 实体工具函数集合
 *
 * 这里保留需要独立编译的非模板工具函数。
 * 模板型搜索/距离工具继续放在 core/EntityUtils.hpp 中。
 */
namespace EntityUtils {

/**
 * @brief 将旧实体类型映射为类型标识符
 * @param type 旧实体类型
 * @return 对应的资源标识符字符串
 */
[[nodiscard]] const char* legacyTypeToTypeId(LegacyEntityType type);

} // namespace EntityUtils

} // namespace mc
