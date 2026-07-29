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
#include <vector>

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
 * 写入 compound body（entries + End），**不含**开头 0x0A 类型字节与 root name。
 * 与 readCompound 对称（readCompound 也不消费 0x0A 前缀）。调用方：
 * item/component/DataComponentPatchWire、backend/java/codecs/JavaPlayCodecsExtended 的 NBT
 * payload 透传。
 */
[[nodiscard]] Result<void> writeCompound(ByteBuf& buf, const mc::nbt::tags::compound_tag& tag);

/**
 * @brief 将复合标签以【根 NBT】线格式序列化为字节向量
 *
 * 对齐 Java `FriendlyByteBuf.writeNbt` = `NbtIo.writeAnyTag`：`0x0A`(compound 类型字节)
 * + 空 root name(`0x00 0x00`) + entries + `0x00`(End)。这是 Java `ByteBufCodecs.TAG`
 * 期望的线格式。
 *
 * 供 `RegistryEntry.data` 等需把完整根 NBT 作为原始字节嵌入协议的场景使用——
 * `registryDataCodec` 直接 writeBytes 该向量，故向量内须已是含 0x0A 前缀的完整根 NBT。
 *
 * 与 writeCompound 的区别：writeCompound 仅写 body（无 0x0A、无 name），用于与
 * readCompound 对称的内部往返；本函数写完整根 NBT，用于发往真 Java 客户端。
 */
[[nodiscard]] std::vector<u8> serializeRootCompoundToBytes(const mc::nbt::tags::compound_tag& tag);

/**
 * @brief 将复合标签以【根 NBT】线格式写入 buf（= writeBytes(serializeRootCompoundToBytes)）
 */
[[nodiscard]] Result<void> writeRootCompound(ByteBuf& buf, const mc::nbt::tags::compound_tag& tag);

/**
 * @brief 从 buf 当前游标读取一个 Java 大端二进制复合标签
 */
[[nodiscard]] Result<std::unique_ptr<mc::nbt::tags::compound_tag>> readCompound(ByteBuf& buf);

/**
 * @brief 跳过 buf 当前游标处的一个 Java 大端二进制复合标签（仅推进游标，不解析语义）
 *
 * 供仅需按 NBT 定界跳过字节的 codec 使用（如 PlayerInfoUpdate 的 displayName 字段：
 * 我方不消费 Component，但需跳过真 Java 对端发来的 NBT compound 以免后续字段错位）。
 */
[[nodiscard]] Result<void> skipCompound(ByteBuf& buf);

} // namespace nbt_io

} // namespace mc::network::buffer
