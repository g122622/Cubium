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
#include "common/network/buffer/NbtIo.hpp"
#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/codec/StreamCodec.hpp"
#include "common/network/codec/StreamCodecs.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
 * @brief 将已序列化的 1.21.11 Component NBT wire 字节写入 buf（无外层长度前缀）
 *
 * 对齐 vanilla `ComponentSerialization.TRUSTED_STREAM_CODEC`：NBT 自定界（StringTag 靠 U16 长度，
 * CompoundTag 靠 0x00 End），`FriendlyByteBuf.writeNbt` = `NbtIo.writeAnyTag` 只写 tagId+payload，
 * **无 VarInt 长度前缀**。区别于 writeOpaque（VarInt(len)+bytes）。
 *
 * 调用方负责把 ITextComponent 序列化为 NBT wire 字节（纯文本折叠为 StringTag 字节，复杂组件为
 * CompoundTag 字节）填入 nbtBytes。业务侧工具见 common/util/text/ComponentNbtSerialization。
 */
inline void writeComponentNbt(B& buf, const std::vector<u8>& nbtBytes)
{
    buf.writeBytes(nbtBytes.data(), nbtBytes.size());
}

/**
 * @brief 从 buf 读取 1.21.11 Component NBT wire 字节（writeComponentNbt 的对称）
 *
 * Component NBT 根可能是 StringTag（0x08，纯文本折叠）或 CompoundTag（0x0A，复杂组件）。本函数
 * 读 tagId 后按类型定界推进游标，返回 [start, end) 区间的原始 wire 字节，供下游（客户端 widget
 * 或真客户端解码）按需解析。tag id 非 0x08/0x0A 视为格式错误。CompoundTag body（entries + 0x00
 * End）由 buffer::nbt_io::skipCompound 推进游标（NbtIo.hpp 定义）。
 */
[[nodiscard]] inline Result<std::vector<u8>> readComponentNbt(B& buf)
{
    const usize start = buf.readPosition();
    u8 tagId = 0;
    MC_TRY_ASSIGN(tagId, buf.readU8());
    if (tagId == 0x08) {
        // StringTag：U16 长度 + UTF8 字节
        u16 len = 0;
        MC_TRY_ASSIGN(len, buf.readU16());
        MC_TRY(buf.readBytes(static_cast<usize>(len))); // 推进游标（字节已在 [start, end) 区间内）
    } else if (tagId == 0x0A) {
        // CompoundTag：body（entries + 0x00 End）。0x0A 已消费，skipCompound 消费 body。
        MC_TRY(buffer::nbt_io::skipCompound(buf));
    } else {
        return Error(ErrorCode::InvalidData,
            "Component NBT: expected StringTag(0x08) or CompoundTag(0x0A), got " + std::to_string(tagId),
            "readComponentNbt");
    }
    const usize end = buf.readPosition();
    const auto& all = buf.bytes();
    return std::vector<u8>(all.begin() + start, all.begin() + end);
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
