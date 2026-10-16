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

#include "common/item/component/FireworkExplosion.hpp"

#include <array>

namespace mc {
namespace item {
namespace component {

namespace {

// 与 FireworkShape 枚举值同序：id 即数组下标（vanilla Shape 声明顺序）。
constexpr std::array<std::string_view, 5> _shapeNames{"small_ball", "large_ball", "star", "creeper", "burst"};

} // namespace

std::string_view fireworkShapeName(FireworkShape shape) noexcept
{
    const auto index = static_cast<usize>(shape);
    return index < _shapeNames.size() ? _shapeNames[index] : _shapeNames[0];
}

std::optional<FireworkShape> fireworkShapeFromName(std::string_view name) noexcept
{
    for (usize i = 0; i < _shapeNames.size(); ++i) {
        if (_shapeNames[i] == name) {
            return static_cast<FireworkShape>(i);
        }
    }
    return std::nullopt;
}

FireworkShape fireworkShapeById(i32 id) noexcept
{
    // 越界回落 SmallBall（对齐 vanilla ByIdMap.OutOfBoundsStrategy.ZERO）
    if (id < 0 || static_cast<usize>(id) >= _shapeNames.size()) {
        return FireworkShape::SmallBall;
    }
    return static_cast<FireworkShape>(id);
}

} // namespace component
} // namespace item
} // namespace mc
