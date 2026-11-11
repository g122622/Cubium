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

#include <string>

namespace mc::blockentity {

/**
 * @brief 把基岩 clone filtered 的 `data` 值翻译成 Java 方块状态属性串
 *
 * 基岩的 `clone ... filtered` 用「方块名 + data 数值」描述要保留的方块，而 Java 侧用
 * 方块状态属性串（如 `[facing=north,half=bottom]`）表达同一件事。本函数负责这层翻译。
 *
 * **当前只覆盖台阶（stairs）**：
 * ```
 * data = weirdo_direction(0-3) + upside_down_bit * 4
 * weirdo_direction: 0=east, 1=west, 2=south, 3=north
 * upside_down_bit: 0=bottom, 1=top
 * ```
 *
 * 未识别的方块名、缺失或非法的 `data` 一律返回空字符串，调用方据此退化为
 * 「只按方块类型过滤」——这是刻意的安全回退，不要改成猜测。
 *
 * TODO: 仅覆盖 stairs。扩展需要基岩各方块的 data 位定义，而本项目与手头的逆向头文件
 *   （LeviLamina）里都没有这些定义（检索 weirdo_direction / upside_down_bit / open_bit /
 *   button_pressed_bit 均无结果），wiki 也没有逐方块的位表。已确认楼梯用的是
 *   "0-3 方向 + bit2 上下"，但活板门/按钮/原木的位布局无法从这里推断——活板门还要编码
 *   open_bit、按钮要编码 button_pressed_bit，位位置靠猜会静默产出错误的方块状态。
 *   因此保留"未识别即返回空串"这一安全回退，等有权威 data 位表再补。
 *
 * @param blockTok 基岩方块名 token（如 `minecraft:stone_stairs`）
 * @param dataTok 基岩 data 值 token（十进制字符串）
 * @return Java 方块状态属性串（形如 `[facing=east,half=bottom]`）；无法翻译时返回空串
 */
[[nodiscard]] std::string bedrockCloneFilterDataToJavaProps(const std::string& blockTok, const std::string& dataTok);

} // namespace mc::blockentity
