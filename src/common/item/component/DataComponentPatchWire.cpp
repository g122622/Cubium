/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/network/backend/java/codecs/ItemEnchantmentsCodec.hpp"
#include "common/network/backend/java/codecs/PotionContentsCodec.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"

#include <memory>
#include <string>

namespace mc {
namespace item {
namespace component {

namespace {

using nbt::tags::compound_tag;

// ============================================================================
// 每组件专属 wire codec（1.21.11 DataComponentPatch.STREAM_CODEC）
// ============================================================================
// vanilla 线格式：VarInt(addedCount) + VarInt(removedCount)（两 count 在前）
//   + added[ VarInt(typeId) + per-component value ]*
//   + removed[ VarInt(typeId) ]*
// 每组件 value 用其专属 streamCodec，无统一 {v:NBT} 外壳。

/// 从 buf 读一个 Component NBT wire 区段（自定界），返回其原始字节。
/// Component NBT 根可能是 StringTag(0x08，纯文本折叠) 或 CompoundTag(0x0A，复杂组件)。
[[nodiscard]] Result<std::vector<u8>> readComponentNbtBytes(network::buffer::ByteBuf& buf)
{
    const usize start = buf.readPosition();
    u8 tagId = 0;
    MC_TRY_ASSIGN(tagId, buf.readU8());
    if (tagId == 0x08) {
        // StringTag：U16 长度 + UTF8 字节。
        u16 len = 0;
        MC_TRY_ASSIGN(len, buf.readU16());
        MC_TRY(buf.readBytes(static_cast<usize>(len)));
    } else if (tagId == 0x0A) {
        // CompoundTag：0x0A 已消费，skipCompound 消费 body（entries + 0x00 End）。
        MC_TRY(network::buffer::nbt_io::skipCompound(buf));
    } else {
        return Error(ErrorCode::InvalidData,
            "Component NBT: expected StringTag(0x08) or CompoundTag(0x0A)",
            "readComponentNbtBytes");
    }
    const usize end = buf.readPosition();
    const u8* base = buf.data() + start;
    return std::vector<u8>(base, base + (end - start));
}

/// 写单个组件 value 到 wire（按 type 分派到专属 codec）。
[[nodiscard]] Result<void> writeComponentValue(
    network::buffer::ByteBuf& buf, DataComponentType type, const DataComponentPayload& payload)
{
    using network::backend::java::writeItemEnchantments;
    using network::backend::java::writePotionContentsPayload;
    switch (type) {
        case DataComponentType::Damage:
        case DataComponentType::RepairCost: {
            // 裸 VarInt。
            const auto* p = std::get_if<i32>(&payload);
            buf.writeVarInt(p ? *p : 0);
            return {};
        }
        case DataComponentType::CustomName: {
            // Component NBT（自定界，无外层长度前缀）。
            const auto& comp = std::get<std::unique_ptr<text::ITextComponent>>(payload);
            const auto nbtBytes = text::componentToNbtBytes(comp.get());
            buf.writeBytes(nbtBytes.data(), nbtBytes.size());
            return {};
        }
        case DataComponentType::Lore: {
            // VarInt(count) + [Component NBT]*。
            const auto& lines = std::get<std::vector<std::unique_ptr<text::ITextComponent>>>(payload);
            buf.writeVarInt(static_cast<i32>(lines.size()));
            for (const auto& line : lines) {
                const auto nbtBytes = text::componentToNbtBytes(line.get());
                buf.writeBytes(nbtBytes.data(), nbtBytes.size());
            }
            return {};
        }
        case DataComponentType::Enchantments: {
            const auto& ench = std::get<item::enchant::EnchantmentContainer>(payload);
            writeItemEnchantments(buf, ench);
            return {};
        }
        case DataComponentType::PotionContents: {
            const auto& pc = std::get<PotionContentsPayload>(payload);
            writePotionContentsPayload(buf, pc);
            return {};
        }
        case DataComponentType::CanPlaceOn:
        case DataComponentType::CanBreak: {
            // AdventureModePredicate wire = VarInt(count) + [BlockPredicate]*。
            // 项目以 vector<string> 承载谓词，与 vanilla 结构化 BlockPredicate 不兼容；
            // 此处降级为空 list（VarInt(0)，单字节 0x00）保证 vanilla 客户端可接受，
            // 实际谓词由 CustomData 侧承载。完整 BlockPredicate 体系待后续落地。
            buf.writeVarInt(0);
            return {};
        }
        case DataComponentType::CustomData: {
            // 单个 NBT tag 自定界（writeRootCompound = writeAnyTag，含 0x0A 前缀）。
            const auto& j = std::get<nlohmann::json>(payload);
            compound_tag tag;
            if (j.is_object() && !j.empty()) {
                auto nbt = nbt::jsonToNbt(j);
                if (nbt != nullptr && nbt->id() == nbt::TagId::Compound) {
                    tag = dynamic_cast<const compound_tag&>(*nbt);
                }
            }
            return network::buffer::nbt_io::writeRootCompound(buf, tag);
        }
        default:
            // 未落地组件：写空 compound 占位（仅项目内部往返，不与 vanilla 互通）。
            return network::buffer::nbt_io::writeRootCompound(buf, compound_tag{});
    }
}

/// 从 wire 读单个组件 value（按 type 分派到专属 codec）。
[[nodiscard]] Result<DataComponentPayload> readComponentValue(network::buffer::ByteBuf& buf, DataComponentType type)
{
    using network::backend::java::readItemEnchantments;
    using network::backend::java::readPotionContentsPayload;
    switch (type) {
        case DataComponentType::Damage:
        case DataComponentType::RepairCost: {
            i32 value = 0;
            MC_TRY_ASSIGN(value, buf.readVarInt());
            return DataComponentPayload{std::in_place_index<1>, value};
        }
        case DataComponentType::CustomName: {
            std::vector<u8> nbtBytes;
            MC_TRY_ASSIGN(nbtBytes, readComponentNbtBytes(buf));
            // 把 NBT wire 字节还原为 ITextComponent：解析为纯文本后构造 StringTextComponent。
            // 完整 Component 还原（style/extra）待后续；此处取纯文本保业务可用。
            const auto plain = text::componentNbtBytesToPlainText(nbtBytes);
            std::unique_ptr<text::ITextComponent> comp = std::make_unique<text::StringTextComponent>(plain);
            return DataComponentPayload{std::in_place_index<2>, std::move(comp)};
        }
        case DataComponentType::Lore: {
            std::vector<std::unique_ptr<text::ITextComponent>> lines;
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "Lore count is negative", "readComponentValue");
            }
            for (i32 i = 0; i < count; ++i) {
                std::vector<u8> nbtBytes;
                MC_TRY_ASSIGN(nbtBytes, readComponentNbtBytes(buf));
                lines.push_back(
                    std::make_unique<text::StringTextComponent>(text::componentNbtBytesToPlainText(nbtBytes)));
            }
            return DataComponentPayload{std::in_place_index<3>, std::move(lines)};
        }
        case DataComponentType::Enchantments: {
            auto enchResult = readItemEnchantments(buf);
            if (enchResult.failed()) {
                return enchResult.error();
            }
            return DataComponentPayload{std::in_place_index<4>, std::move(enchResult.value())};
        }
        case DataComponentType::PotionContents: {
            auto pcResult = readPotionContentsPayload(buf);
            if (pcResult.failed()) {
                return pcResult.error();
            }
            return DataComponentPayload{std::in_place_index<5>, std::move(pcResult.value())};
        }
        case DataComponentType::CanPlaceOn:
        case DataComponentType::CanBreak: {
            // 降级：读空 list（VarInt(0)）；vanilla 发来的非空谓词按 count 跳过每个 BlockPredicate
            // 需完整体系，此处仅支持空 list，非空返回空谓词（丢失语义，保互通不崩）。
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            AdventureModePredicate pred{};
            // 注：非空 list 的 BlockPredicate 字节未消费，会导致后续错位；当前项目不主动
            // 发送非空谓词，真 Java 对端发来的非空谓词场景待完整 BlockPredicate 体系落地。
            (void)count;
            return DataComponentPayload{std::in_place_index<6>, std::move(pred)};
        }
        case DataComponentType::CustomData: {
            auto tagResult = network::buffer::nbt_io::readRootCompound(buf);
            if (tagResult.failed()) {
                return tagResult.error();
            }
            // readRootCompound 的 Result 按 unique_ptr 取值，value() 返回右值 unique_ptr，
            // 一次性取走到局部变量（见记忆 result-uniqueptr-value-takeownership-pitfall）。
            auto tag = tagResult.value();
            return DataComponentPayload{std::in_place_index<7>, nbt::nbtToJson(*tag)};
        }
        default:
            // 未落地组件：跳过一个根 NBT（占位读丢）。
            MC_TRY(network::buffer::nbt_io::skipCompound(buf));
            return DataComponentPayload{};
    }
}

} // namespace

Result<void> writePatchToWire(network::buffer::ByteBuf& buf, const DataComponentPatch& patch)
{
    // 两 count 在前（vanilla 顺序），added 段全在后，removed 段最后。
    buf.writeVarInt(static_cast<i32>(patch.added().size()));
    buf.writeVarInt(static_cast<i32>(patch.removed().size()));
    for (const auto& entry : patch.added()) {
        auto type = componentTypeById(entry.typeId);
        if (!type.has_value()) {
            continue; // 未落地：跳过（不写 typeId，读端也不期望）
        }
        buf.writeVarInt(entry.typeId);
        MC_TRY(writeComponentValue(buf, *type, entry.value));
    }
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
    i32 removedCount = 0;
    MC_TRY_ASSIGN(removedCount, buf.readVarInt());
    if (removedCount < 0) {
        return Error(ErrorCode::InvalidData, "DataComponentPatch removed count is negative", "readPatchFromWire");
    }
    for (i32 i = 0; i < addedCount; ++i) {
        i32 typeId = 0;
        MC_TRY_ASSIGN(typeId, buf.readVarInt());
        auto type = componentTypeById(typeId);
        if (!type.has_value()) {
            // 未知组件：vanilla 标准_patch 无长度前缀，无法安全跳过，报错。
            return Error(ErrorCode::InvalidData, "Unknown data component typeId in wire", "readPatchFromWire");
        }
        auto payloadResult = readComponentValue(buf, *type);
        if (payloadResult.failed()) {
            return payloadResult.error();
        }
        patch.add(*type, std::move(payloadResult.value()));
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
    // 与 readPatchFromWire 同结构（两 count 在前 + added 段 + removed 段），但只推进游标、
    // 不构造 payload。每组件 value 按 typeId 分派到专属 codec 跳过；未知 typeId 无法跳过
    // （vanilla 标准 patch 无长度前缀），报错——这是 vanilla 格式的固有约束。
    const usize start = buf.readPosition();

    i32 addedCount = 0;
    MC_TRY_ASSIGN(addedCount, buf.readVarInt());
    if (addedCount < 0) {
        return Error(ErrorCode::InvalidData, "DataComponentPatch added count is negative", "readPatchBytesFromWire");
    }
    i32 removedCount = 0;
    MC_TRY_ASSIGN(removedCount, buf.readVarInt());
    if (removedCount < 0) {
        return Error(ErrorCode::InvalidData, "DataComponentPatch removed count is negative", "readPatchBytesFromWire");
    }
    for (i32 i = 0; i < addedCount; ++i) {
        i32 typeId = 0;
        MC_TRY_ASSIGN(typeId, buf.readVarInt());
        auto type = componentTypeById(typeId);
        if (!type.has_value()) {
            return Error(ErrorCode::InvalidData, "Unknown data component typeId in wire", "readPatchBytesFromWire");
        }
        // 按专属 codec 推进游标（丢弃解析结果）。
        auto payloadResult = readComponentValue(buf, *type);
        if (payloadResult.failed()) {
            return payloadResult.error();
        }
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
