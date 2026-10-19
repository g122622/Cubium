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

#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/util/TriState.hpp"
#include "common/world/level/MoonPhase.hpp"

#include <nlohmann/json_fwd.hpp>

#include <functional>
#include <string_view>

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 值类型的数据包 JSON 解析器工厂
 *
 * 主模板故意不提供定义：引用它会导致链接错误，从而在编译期之外尽早暴露"新增了
 * 没有 JSON 表示的值类型却仍被数据包引用"。
 *
 * 已支持的类型（定义见 .cpp 中的显式特化）：bool / f32 / i32（颜色或整数）/
 * util::TriState / MoonPhase / Activity。
 *
 * 注意：调用方必须通过函数指针间接调用（见 `EnvironmentAttributeRegistry::registerAttribute`），
 * 否则编译器会在调用点按主模板实例化。
 */
template <typename Value>
[[nodiscard]] std::function<bool(const void* json, Value& out)> valueJsonReader();

} // namespace attribute
} // namespace world
} // namespace mc