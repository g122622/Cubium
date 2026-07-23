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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"

#include <memory>

namespace mc::nbt::tags {
struct compound_tag;
} // namespace mc::nbt::tags

namespace mc::network::buffer {

class ByteBuf;

/**
 * @brief ByteBuf ↔ NBT 桥接
 *
 * nbt 库基于 std::istream/ostream（带 Context 字节序上下文），不直接读写 ByteBuf。
 * 本桥接用 std::stringstream 中转：写时把 tag 序列化进 stringstream 再整体追加到 ByteBuf，
 * 读时从 ByteBuf 切出 NBT 子串喂给 stringstream 解析。
 *
 * Java 线协议的 NBT/ComponentNbt 全程大端二进制（Contexts::java）。
 */
namespace nbt_io {

/**
 * @brief 将复合标签以 Java 大端二进制写入 buf
 *
 * TODO(Phase3/Phase5): 1.21.11 物品组件用 DataComponentPatch，其内嵌 NBT 走本桥接。
 */
[[nodiscard]] Result<void> writeCompound(ByteBuf& buf, const mc::nbt::tags::compound_tag& tag);

/**
 * @brief 从 buf 当前游标读取一个 Java 大端二进制复合标签
 */
[[nodiscard]] Result<std::unique_ptr<mc::nbt::tags::compound_tag>> readCompound(ByteBuf& buf);

} // namespace nbt_io

} // namespace mc::network::buffer
