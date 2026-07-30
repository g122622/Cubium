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

#include "common/item/component/DataComponentPatchWire.hpp"

#include "common/item/component/DataComponentPayloadCodec.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <memory>
#include <string>

namespace mc {
namespace item {
namespace component {

namespace {

using nbt::tags::compound_tag;

// 把单个组件 payload 包进名为 "v" 的 compound，经 nbt_io 大端写出。
[[nodiscard]] Result<void> writeComponentValue(
    network::buffer::ByteBuf& buf, DataComponentType type, const DataComponentPayload& payload)
{
    auto valueTag = detail::payloadToNbt(type, payload);
    compound_tag wrapper;
    wrapper.value.emplace("v", std::move(valueTag));
    return network::buffer::nbt_io::writeCompound(buf, wrapper);
}

// 从 buf 读一个组件值 NBT，取出 "v" 子 tag，转回 payload。读失败返回错误。
[[nodiscard]] Result<DataComponentPayload> readComponentValue(network::buffer::ByteBuf& buf, DataComponentType type)
{
    auto wrapperResult = network::buffer::nbt_io::readCompound(buf);
    if (wrapperResult.failed()) {
        return wrapperResult.error();
    }
    auto wrapper = wrapperResult.value();
    auto it = wrapper->value.find("v");
    if (it == wrapper->value.end()) {
        // 无 "v" 键：返回该类型的默认 payload（用空 compound 取默认）
        return detail::nbtToPayload(type, compound_tag{});
    }
    return detail::nbtToPayload(type, *it->second);
}

} // namespace

Result<void> writePatchToWire(network::buffer::ByteBuf& buf, const DataComponentPatch& patch)
{
    buf.writeVarInt(static_cast<i32>(patch.added().size()));
    for (const auto& entry : patch.added()) {
        auto type = componentTypeById(entry.typeId);
        if (!type.has_value()) {
            continue; // 未落地：跳过（不写 typeId，读端也不期望）
        }
        buf.writeVarInt(entry.typeId);
        MC_TRY(writeComponentValue(buf, *type, entry.value));
    }
    buf.writeVarInt(static_cast<i32>(patch.removed().size()));
    for (i32 typeId : patch.removed()) {
        buf.writeVarInt(typeId);
    }
    return {};
}

Result<DataComponentPatch> readPatchFromWire(network::buffer::ByteBuf& buf)
{
    DataComponentPatch patch;
    i32 addedCount = 0;
    MC_TRY_ASSIGN(addedCount, buf.readVarInt());
    if (addedCount < 0) {
        return Error(ErrorCode::InvalidData, "DataComponentPatch added count is negative", "readPatchFromWire");
    }
    for (i32 i = 0; i < addedCount; ++i) {
        i32 typeId = 0;
        MC_TRY_ASSIGN(typeId, buf.readVarInt());
        auto type = componentTypeById(typeId);
        if (!type.has_value()) {
            // 未知组件：无法安全跳过其 NBT（长度不定），报错。
            return Error(ErrorCode::InvalidData, "Unknown data component typeId in wire", "readPatchFromWire");
        }
        auto payloadResult = readComponentValue(buf, *type);
        if (payloadResult.failed()) {
            return payloadResult.error();
        }
        patch.add(*type, std::move(payloadResult.value()));
    }
    i32 removedCount = 0;
    MC_TRY_ASSIGN(removedCount, buf.readVarInt());
    if (removedCount < 0) {
        return Error(ErrorCode::InvalidData, "DataComponentPatch removed count is negative", "readPatchFromWire");
    }
    for (i32 i = 0; i < removedCount; ++i) {
        i32 typeId = 0;
        MC_TRY_ASSIGN(typeId, buf.readVarInt());
        patch.remove(typeId);
    }
    return patch;
}

Result<std::vector<u8>> readPatchBytesFromWire(network::buffer::ByteBuf& buf)
{
    // 记录 patch 区段起点，按 vanilla 自终止结构推进游标，最后切出消费到的字节副本。
    const usize start = buf.readPosition();

    i32 addedCount = 0;
    MC_TRY_ASSIGN(addedCount, buf.readVarInt());
    if (addedCount < 0) {
        return Error(ErrorCode::InvalidData, "DataComponentPatch added count is negative", "readPatchBytesFromWire");
    }
    for (i32 i = 0; i < addedCount; ++i) {
        MC_TRY(buf.readVarInt()); // typeId（不查表，原样跳过）
        // value：与 writeComponentValue 的 writeCompound（body，无 0x0A 前缀）对称，按 NBT compound 定界跳过。
        MC_TRY(network::buffer::nbt_io::skipCompound(buf));
    }
    i32 removedCount = 0;
    MC_TRY_ASSIGN(removedCount, buf.readVarInt());
    if (removedCount < 0) {
        return Error(ErrorCode::InvalidData, "DataComponentPatch removed count is negative", "readPatchBytesFromWire");
    }
    for (i32 i = 0; i < removedCount; ++i) {
        MC_TRY(buf.readVarInt()); // typeId
    }

    const usize end = buf.readPosition();
    const u8* base = buf.data() + start;
    return std::vector<u8>(base, base + (end - start));
}

} // namespace component
} // namespace item
} // namespace mc
