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

#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/codec/StreamCodec.hpp"
#include "common/network/codec/StreamCodecs.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace mc::network::backend::java::codecs {

/// Java 后端 codec 使用的缓冲类型别名（RegistryFriendlyByteBuf 的 Java 对应）。
using B = buffer::RegistryByteBuf;

/**
 * @brief 将纯文本 Component 以 NBT StringTag 线格式写入 buf
 *
 * 对齐 vanilla `ComponentSerialization` 纯文本折叠路径：`Component.literal(text)` 经
 * `Codec.encode(NbtOps)` → `StringTag`，再经 `FriendlyByteBuf.writeNbt` = `NbtIo.writeAnyTag`
 * 写出。线格式 = `0x08`(StringTag tag id) + `U16`(UTF8 字节数，大端) + UTF8 字节，
 * **无 root name 前缀**（writeAnyTag 只写 tagId + payload，区别于磁盘 NBT 的 writeUnnamedTag）。
 * StringTag 长度是 2 字节大端 unsigned short（`DataOutput.writeUTF`），**非 VarInt**。
 *
 * 供 Disconnect（Login/Configuration/Play 三阶段共用同一包类）的 reason 字段使用——
 * vanilla `ClientboundDisconnectPacket.reason` 是 `Component`，不是裸字符串。
 */
inline void writeTextComponentNbt(B& buf, std::string_view text)
{
    buf.writeU8(0x08); // StringTag tag id
    const usize len = text.size();
    buf.writeU16(static_cast<u16>(len));
    if (len > 0) {
        buf.writeBytes(reinterpret_cast<const u8*>(text.data()), len);
    }
}

/**
 * @brief 从 buf 读取 NBT StringTag 线格式的纯文本 Component（writeTextComponentNbt 的对称）
 *
 * 校验并消费 `0x08` tag id + `U16` 长度 + UTF8 字节。tag id 非 0x08 视为格式错误。
 */
[[nodiscard]] inline Result<std::string> readTextComponentNbt(B& buf)
{
    u8 tagId = 0;
    MC_TRY_ASSIGN(tagId, buf.readU8());
    if (tagId != 0x08) {
        return Error(ErrorCode::InvalidData, "TextComponent NBT: expected StringTag(0x08)", "readTextComponentNbt");
    }
    u16 len = 0;
    MC_TRY_ASSIGN(len, buf.readU16());
    if (len == 0) {
        return std::string{};
    }
    auto viewResult = buf.readBytesView(static_cast<usize>(len));
    if (!viewResult.success()) {
        return viewResult.error();
    }
    return std::string(viewResult.value().data(), viewResult.value().size());
}

/**
 * @brief 用 lambda 构造 StreamCodec<B, V>（按值持有，可被 ProtocolInfoBuilder addPacket 接收）
 *
 * 各阶段 codec 头文件经此工具组装字段级 encode/decode lambda。抽出本文件，使
 * JavaConfigurationCodecs.hpp / JavaPlayCodecs.hpp 可独立包含，无需依赖 JavaCodecs.hpp
 * 的定义顺序。
 */
template <typename V, typename EncodeFn, typename DecodeFn>
[[nodiscard]] auto makeCodec(EncodeFn encodeFn, DecodeFn decodeFn)
{
    return codec::makeLambdaCodec<B, V>(std::move(encodeFn), std::move(decodeFn));
}

} // namespace mc::network::backend::java::codecs
