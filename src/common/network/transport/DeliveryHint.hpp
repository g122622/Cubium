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

namespace mc::network::transport {

/**
 * @brief 投递语义提示（对应基岩版 RakNet 的 reliability 等级）
 *
 * TCP 后端（Java）全部按 ReliableOrdered 处理（TCP 本身保证可靠有序）；
 * RakNet 后端按此提示选择 reliability/ordering channel。
 * Local 后端（同进程）忽略提示直接同步投递。
 */
enum class DeliveryHint : u8 {
    Unreliable,          // 不保证送达、不保证顺序（RakNet 0）
    UnreliableSequenced, // 不保证送达但按序（丢旧保新）
    Reliable,            // 保证送达、不保证顺序
    ReliableOrdered,     // 保证送达且按序（默认）
};

} // namespace mc::network::transport
