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

#include <cstddef>

namespace mc {

class BlockPos;
class BlockState;
class IWorld;

namespace world::map {

/**
 * @brief 取地图着色应该使用的方块状态
 *
 * 含水方块（waterlogged 等）若顶面不是完整支撑面，地图上应显示成水而不是它本身的颜色；
 * 做法是把状态换成流体的 legacy 方块（水 → 水方块）再取色。顶面完整的含水方块
 * （如朝上的 waterlogged 石台阶）仍显示自身颜色。
 *
 * 这是地图取色链路的纯函数部分，从 `FilledMapItem` 中抽出：它不依赖地图物品的任何状态，
 * 抽出来后可以被单元测试直接覆盖（此前挂在 `FilledMapItem` 的私有静态成员上，
 * 测试只能通过 friend 或很重的公开路径触达）。
 *
 * @param world 世界（`isFaceSturdy` 需要世界参数）
 * @param state 方块状态
 * @param pos 方块坐标
 * @return 用于着色的状态；不需要替换时返回 `&state` 本身
 */
[[nodiscard]] const BlockState* mapColorStateFor(IWorld& world, const BlockState& state, const BlockPos& pos);

} // namespace world::map
} // namespace mc
