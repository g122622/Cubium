#pragma once

#include "common/entity/effect/EffectType.hpp"

#include <optional>

namespace mc::command::support {

/**
 * @brief 按命令名称解析状态效果类型。
 *
 * @param name 命令中输入的效果名。
 * @return 匹配到的效果类型；无法解析时返回空值。
 *
 * @note 该解析层使用 Java 版常见蛇形命名，如 `night_vision`、`hero_of_the_village`。
 */
[[nodiscard]] std::optional<entity::effect::EffectType> tryParseEffectType(StringView name) noexcept;

/**
 * @brief 获取命令输出中使用的效果名称。
 *
 * @param type 效果类型。
 * @return 命令风格的蛇形名称。
 */
[[nodiscard]] const char* getEffectCommandName(entity::effect::EffectType type) noexcept;

} // namespace mc::command::support
