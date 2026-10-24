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

#include "common/core/Types.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace mc {
enum class DamageType : u8;
}

namespace mc::mod::bedrock::addon {

/** @brief 将游戏伤害类型转换为官方脚本伤害原因。 */
[[nodiscard]] std::string_view damageCauseName(mc::DamageType type);
/** @brief 将游戏伤害类型转换为 JSON 过滤器使用的下划线命名。 */
[[nodiscard]] std::string damageFilterCauseName(mc::DamageType type);
/** @brief 将脚本伤害原因解析为游戏伤害类型。 */
[[nodiscard]] std::optional<mc::DamageType> parseDamageCause(std::string_view name);

} // namespace mc::mod::bedrock::addon
