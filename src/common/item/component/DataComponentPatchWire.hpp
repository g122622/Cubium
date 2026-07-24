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
#include "common/item/component/DataComponentMap.hpp"

namespace mc {
namespace network {
namespace buffer {
class ByteBuf;
} // namespace buffer
} // namespace network

namespace item {
namespace component {

/**
 * @brief 将 DataComponentPatch 写入 Java wire 缓冲
 *
 * 线格式（本项目落地版，对齐 1.21.11 DataComponentPatch.STREAM_CODEC 骨架）：
 *   VarInt(addedCount)
 *   + [ VarInt(typeId) + ComponentNBT(value) ]*    // 每个值经 nbt_io 桥接为大端 NBT compound
 *   + VarInt(removedCount)
 *   + [ VarInt(typeId) ]*
 *
 * 每个 added 值的 ComponentNBT 编码：把 payload 序列化为单个 NBT tag 后，包进一个
 * 名为 "v" 的 compound，再以 Java 大端二进制写出（长度前缀由 nbt_io::writeCompound 处理）。
 * 这保证了我方客户端↔服务端互通（必达）；与真 Java 1.21.11 每组件专属 codec 的差异标 TODO(Phase6)。
 *
 * 未落地 typeId 不写出（调用方保证）。
 *
 * @param buf 目标缓冲
 * @param patch 组件补丁
 * @return 成功或错误
 */
[[nodiscard]] Result<void> writePatchToWire(network::buffer::ByteBuf& buf, const DataComponentPatch& patch);

/**
 * @brief 从 Java wire 缓冲读取 DataComponentPatch
 *
 * 线格式见 writePatchToWire。未知 typeId 跳过其 NBT 值后继续（容错）。
 *
 * @param buf 源缓冲
 * @return 解析出的 patch 或错误
 */
[[nodiscard]] Result<DataComponentPatch> readPatchFromWire(network::buffer::ByteBuf& buf);

} // namespace component
} // namespace item
} // namespace mc
