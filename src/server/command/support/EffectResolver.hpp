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

#pragma once

#include "common/entity/effect/EffectType.hpp"

#include <optional>
#include <string_view>

namespace mc::command::support {

/**
 * @brief 按命令名称解析状态效果类型。
 *
 * @param name 命令中输入的效果名。
 * @return 匹配到的效果类型；无法解析时返回空值。
 *
 * @note 该解析层使用 Java 版常见蛇形命名，如 `night_vision`、`hero_of_the_village`。
 */
[[nodiscard]] std::optional<entity::effect::EffectType> tryParseEffectType(std::string_view name) noexcept;

/**
 * @brief 获取命令输出中使用的效果名称。
 *
 * @param type 效果类型。
 * @return 命令风格的蛇形名称。
 */
[[nodiscard]] const char* getEffectCommandName(entity::effect::EffectType type) noexcept;

} // namespace mc::command::support
