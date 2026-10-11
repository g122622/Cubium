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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/component/AttackRange.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/ItemRarity.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/item/items/special/bundle/BundleContents.hpp"
#include "common/network/backend/java/codecs/ItemEnchantmentsCodec.hpp"
#include "common/network/backend/java/codecs/PotionContentsCodec.hpp"
#include "common/network/backend/java/mappings/JavaBannerPatternIds.hpp"
#include "common/network/backend/java/mappings/JavaItemIdMap.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/util/crypto/Crc32c.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/Utf8.hpp"
#include "common/world/blockentity/interactive/BannerPattern.hpp"

#include <memory>
#include <span>
#include <utility>
#include <variant>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace item {
namespace component {

namespace {

thread_local usize _patchDepth = 0;
struct _PatchDepthGuard {
    _PatchDepthGuard() { ++_patchDepth; }
    ~_PatchDepthGuard() { --_patchDepth; }
};

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

// Filterable 的未过滤形式是裸值，带过滤形式为 {raw, filtered}。
const nlohmann::json& _rawBookValue(const nlohmann::json& value)
{
    return value.is_object() && value.contains("raw") ? value.at("raw") : value;
}

Result<void> _writeBookValue(
    network::buffer::ByteBuf& buf, const nlohmann::json& value, bool component, usize maxLength)
{
    auto write = [&](const nlohmann::json& raw) -> Result<void> {
        if (component) {
            const auto parsed = text::ITextComponent::fromJson(raw);
            const auto bytes = text::componentToNbtBytes(parsed.get());
            buf.writeBytes(bytes.data(), bytes.size());
        } else {
            const auto& string = raw.get_ref<const std::string&>();
            const auto length = util::text::utf16Length(string);
            if (!length || *length > maxLength) return Error(ErrorCode::InvalidData, "Invalid book string");
            buf.writeString(string);
        }
        return {};
    };
    MC_TRY(write(_rawBookValue(value)));
    const bool filtered = value.is_object() && value.contains("filtered");
    buf.writeBool(filtered);
    if (filtered) MC_TRY(write(value.at("filtered")));
    return {};
}

Result<nlohmann::json> _readBookValue(network::buffer::ByteBuf& buf, bool component, usize maxLength)
{
    auto read = [&]() -> Result<nlohmann::json> {
        if (component) {
            std::vector<u8> bytes;
            MC_TRY_ASSIGN(bytes, readComponentNbtBytes(buf));
            if (bytes.front() == 8) return nlohmann::json(text::componentNbtBytesToPlainText(bytes));
            network::buffer::ByteBuf nbtBuf(bytes.data(), bytes.size());
            auto tagResult = network::buffer::nbt_io::readRootCompound(nbtBuf);
            if (tagResult.failed()) return tagResult.error();
            const auto tag = tagResult.value();
            return nbt::nbtToJson(*tag);
        }
        std::string value;
        MC_TRY_ASSIGN(value, buf.readString());
        const auto length = util::text::utf16Length(value);
        if (!length || *length > maxLength) return Error(ErrorCode::InvalidData, "Invalid book string");
        return nlohmann::json(value);
    };
    nlohmann::json raw;
    MC_TRY_ASSIGN(raw, read());
    bool filtered = false;
    MC_TRY_ASSIGN(filtered, buf.readBool());
    if (!filtered) return raw;
    nlohmann::json replacement;
    MC_TRY_ASSIGN(replacement, read());
    return nlohmann::json{{"raw", std::move(raw)}, {"filtered", std::move(replacement)}};
}

/// 写单个组件 value 到 wire（按 type 分派到专属 codec）。
[[nodiscard]] Result<void> _writeComponentValue(
    network::buffer::ByteBuf& buf, DataComponentType type, const DataComponentPayload& payload)
{
    using network::backend::java::writeItemEnchantments;
    using network::backend::java::writePotionContentsPayload;
    switch (type) {
        case DataComponentType::Damage:
        case DataComponentType::RepairCost:
        case DataComponentType::MaxStackSize:
        case DataComponentType::MaxDamage:
        case DataComponentType::Enchantable: {
            // 裸 VarInt。
            const auto* p = std::get_if<i32>(&payload);
            buf.writeVarInt(p ? *p : 0);
            return {};
        }
        case DataComponentType::Unbreakable: {
            // Unit.STREAM_CODEC 写 0 字节。
            return {};
        }
        case DataComponentType::Rarity: {
            const auto* p = std::get_if<ItemRarity>(&payload);
            buf.writeVarInt(rarityId(p ? *p : ItemRarity::Common));
            return {};
        }
        case DataComponentType::ItemModel: {
            const auto* p = std::get_if<std::string>(&payload);
            buf.writeString(p ? *p : std::string{});
            return {};
        }
        case DataComponentType::AttackRange: {
            const auto& ar = std::get<AttackRange>(payload);
            buf.writeF32(ar.minRange);
            buf.writeF32(ar.maxRange);
            buf.writeF32(ar.minCreativeRange);
            buf.writeF32(ar.maxCreativeRange);
            buf.writeF32(ar.hitboxMargin);
            buf.writeF32(ar.mobFactor);
            return {};
        }
        case DataComponentType::CustomName:
        case DataComponentType::ItemName: {
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
        case DataComponentType::StoredEnchantments:
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
            // TODO: 此处降级为空 list（VarInt(0)，单字节 0x00）保证 vanilla 客户端可接受，
            // 实际谓词由 CustomData 侧承载。完整 BlockPredicate 体系待后续落地。
            buf.writeVarInt(0);
            return {};
        }
        case DataComponentType::BundleContents: {
            const auto contents = items::BundleContents::fromJson(std::get<nlohmann::json>(payload));
            buf.writeVarInt(static_cast<i32>(contents.size()));
            for (const auto& stack : contents.items()) {
                buf.writeVarInt(stack.getCount());
                buf.writeVarInt(static_cast<i32>(
                    network::backend::java::JavaItemIdMap::instance().toJavaRegistryId(*stack.getItem())));
                MC_TRY(writePatchToWire(buf, stack.toComponentPatch()));
            }
            return {};
        }
        case DataComponentType::WritableBookContent:
        case DataComponentType::WrittenBookContent: {
            const auto& book = std::get<nlohmann::json>(payload);
            const bool written = type == DataComponentType::WrittenBookContent;
            if (written) {
                MC_TRY(_writeBookValue(buf, book.at("title"), false, 32));
                buf.writeString(book.at("author").get<std::string>());
                buf.writeVarInt(book.value("generation", 0));
            }
            const auto pages = book.value("pages", nlohmann::json::array());
            if (!written && pages.size() > 100) return Error(ErrorCode::InvalidData, "Too many book pages");
            buf.writeVarInt(static_cast<i32>(pages.size()));
            for (const auto& page : pages)
                MC_TRY(_writeBookValue(buf, page, written, 1024));
            if (written) buf.writeBool(book.value("resolved", false));
            return {};
        }
        case DataComponentType::BannerPatterns: {
            const auto& layers = std::get<nlohmann::json>(payload);
            buf.writeVarInt(static_cast<i32>(layers.size()));
            for (const auto& layer : layers) {
                if (layer.contains("DirectPattern")) {
                    buf.writeVarInt(0);
                    buf.writeString(layer["DirectPattern"].at("asset_id").get<std::string>());
                    buf.writeString(layer["DirectPattern"].at("translation_key").get<std::string>());
                } else {
                    const auto type = blockentity::BannerPatterns::byHash(layer.at("Pattern").get<std::string>());
                    const auto id = network::backend::java::JavaBannerPatternIds::toWireId(
                        "minecraft:" + blockentity::BannerPatterns::getFileName(type));
                    if (!id) return Error(ErrorCode::InvalidData, "Unknown banner pattern");
                    buf.writeVarInt(*id + 1);
                }
                const auto color = layer.at("Color").get<i32>();
                if (color < 0 || color >= 16) return Error(ErrorCode::InvalidData, "Invalid banner color");
                buf.writeVarInt(color);
            }
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
    }
    // 枚举已全覆盖，此处不可达（保留以满足 -Wreturn-type）。
    return {};
}

/// 从 wire 读单个组件 value（按 type 分派到专属 codec）。
[[nodiscard]] Result<DataComponentPayload> _readComponentValue(network::buffer::ByteBuf& buf, DataComponentType type)
{
    using network::backend::java::readItemEnchantments;
    using network::backend::java::readPotionContentsPayload;
    switch (type) {
        case DataComponentType::Damage:
        case DataComponentType::RepairCost:
        case DataComponentType::MaxStackSize:
        case DataComponentType::MaxDamage:
        case DataComponentType::Enchantable: {
            i32 value = 0;
            MC_TRY_ASSIGN(value, buf.readVarInt());
            return DataComponentPayload{std::in_place_index<1>, value};
        }
        case DataComponentType::Unbreakable: {
            // Unit.STREAM_CODEC 读 0 字节。
            return DataComponentPayload{std::in_place_index<8>, true};
        }
        case DataComponentType::Rarity: {
            i32 id = 0;
            MC_TRY_ASSIGN(id, buf.readVarInt());
            return DataComponentPayload{std::in_place_index<9>, rarityFromId(id)};
        }
        case DataComponentType::ItemModel: {
            std::string model;
            MC_TRY_ASSIGN(model, buf.readString());
            return DataComponentPayload{std::in_place_index<10>, std::move(model)};
        }
        case DataComponentType::AttackRange: {
            AttackRange ar{};
            MC_TRY_ASSIGN(ar.minRange, buf.readF32());
            MC_TRY_ASSIGN(ar.maxRange, buf.readF32());
            MC_TRY_ASSIGN(ar.minCreativeRange, buf.readF32());
            MC_TRY_ASSIGN(ar.maxCreativeRange, buf.readF32());
            MC_TRY_ASSIGN(ar.hitboxMargin, buf.readF32());
            MC_TRY_ASSIGN(ar.mobFactor, buf.readF32());
            return DataComponentPayload{std::in_place_index<11>, ar};
        }
        case DataComponentType::CustomName:
        case DataComponentType::ItemName: {
            std::vector<u8> nbtBytes;
            MC_TRY_ASSIGN(nbtBytes, readComponentNbtBytes(buf));
            // 把 NBT wire 字节还原为 ITextComponent：解析为纯文本后构造 StringTextComponent。
            // TODO: 完整 Component 还原（style/extra）待后续；此处取纯文本保业务可用。
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
        case DataComponentType::StoredEnchantments:
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
            // TODO: 需完整体系，此处仅支持空 list，非空返回空谓词（丢失语义，保互通不崩）。
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            AdventureModePredicate pred{};
            // 注：非空 list 的 BlockPredicate 字节未消费，会导致后续错位；当前项目不主动
            // 发送非空谓词，真 Java 对端发来的非空谓词场景待完整 BlockPredicate 体系落地。
            (void)count;
            return DataComponentPayload{std::in_place_index<6>, std::move(pred)};
        }
        case DataComponentType::BundleContents: {
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0 || static_cast<usize>(count) > buf.readableBytes() / 4)
                return Error(ErrorCode::InvalidData, "Invalid bundle item count");
            std::vector<ItemStack> contents;
            for (i32 i = 0; i < count; ++i) {
                i32 amount = 0, id = 0;
                MC_TRY_ASSIGN(amount, buf.readVarInt());
                if (amount <= 0) return Error(ErrorCode::InvalidData, "Empty bundle item");
                MC_TRY_ASSIGN(id, buf.readVarInt());
                auto* item = ItemRegistry::instance().getItem(
                    network::backend::java::JavaItemIdMap::instance().fromJavaRegistryId(static_cast<u32>(id)));
                if (item == nullptr) return Error(ErrorCode::InvalidData, "Unknown bundle item");
                ItemStack stack(item, amount);
                auto patch = readPatchFromWire(buf);
                if (patch.failed()) return patch.error();
                stack.applyComponentPatch(patch.value());
                contents.push_back(std::move(stack));
            }
            return DataComponentPayload{
                std::in_place_index<ComponentPayloadIndex::Json>, items::BundleContents(std::move(contents)).toJson()};
        }
        case DataComponentType::WritableBookContent:
        case DataComponentType::WrittenBookContent: {
            auto book = nlohmann::json::object();
            const bool written = type == DataComponentType::WrittenBookContent;
            if (written) {
                nlohmann::json title;
                MC_TRY_ASSIGN(title, _readBookValue(buf, false, 32));
                std::string author;
                MC_TRY_ASSIGN(author, buf.readString());
                i32 generation = 0;
                MC_TRY_ASSIGN(generation, buf.readVarInt());
                if (generation < 0 || generation > 3) return Error(ErrorCode::InvalidData, "Invalid book generation");
                book = {{"title", std::move(title)}, {"author", std::move(author)}, {"generation", generation}};
            }
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0 || (!written && count > 100) || static_cast<usize>(count) > buf.readableBytes())
                return Error(ErrorCode::InvalidData, "Invalid book page count");
            auto pages = nlohmann::json::array();
            for (i32 i = 0; i < count; ++i) {
                nlohmann::json page;
                MC_TRY_ASSIGN(page, _readBookValue(buf, written, 1024));
                pages.push_back(std::move(page));
            }
            book["pages"] = std::move(pages);
            if (written) {
                bool resolved = false;
                MC_TRY_ASSIGN(resolved, buf.readBool());
                book["resolved"] = resolved;
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, std::move(book)};
        }
        case DataComponentType::BannerPatterns: {
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0 || static_cast<usize>(count) > buf.readableBytes() / 2)
                return Error(ErrorCode::InvalidData, "Invalid banner layer count");
            auto layers = nlohmann::json::array();
            for (i32 i = 0; i < count; ++i) {
                i32 id = 0;
                MC_TRY_ASSIGN(id, buf.readVarInt());
                auto layer = nlohmann::json::object();
                if (id == 0) {
                    std::string asset, key;
                    MC_TRY_ASSIGN(asset, buf.readString());
                    MC_TRY_ASSIGN(key, buf.readString());
                    layer["DirectPattern"] = {{"asset_id", asset}, {"translation_key", key}};
                    layer["Pattern"] = "b";
                } else {
                    const auto& entries = network::backend::java::JavaBannerPatternIds::ENTRIES;
                    if (id < 1 || id > static_cast<i32>(entries.size()))
                        return Error(ErrorCode::InvalidData, "Invalid banner pattern holder");
                    for (i32 type = 0; type < static_cast<i32>(blockentity::BannerPatternType::Count); ++type) {
                        const auto pattern = static_cast<blockentity::BannerPatternType>(type);
                        if ("minecraft:" + blockentity::BannerPatterns::getFileName(pattern) == entries[id - 1]) {
                            layer["Pattern"] = blockentity::BannerPatterns::getHashName(pattern);
                            break;
                        }
                    }
                }
                i32 color = 0;
                MC_TRY_ASSIGN(color, buf.readVarInt());
                if (color < 0 || color >= 16) return Error(ErrorCode::InvalidData, "Invalid banner color");
                layer["Color"] = color;
                layers.push_back(std::move(layer));
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, std::move(layers)};
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
    }
    // 枚举已全覆盖，此处不可达（保留以满足 -Wreturn-type）。
    return DataComponentPayload{};
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
        MC_TRY(_writeComponentValue(buf, *type, entry.value));
    }
    for (i32 typeId : patch.removed()) {
        buf.writeVarInt(typeId);
    }
    return {};
}

Result<DataComponentPatch> readPatchFromWire(network::buffer::ByteBuf& buf)
{
    if (_patchDepth >= 64) return Error(ErrorCode::InvalidData, "Item component nesting exceeds limit");
    _PatchDepthGuard guard;
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
        auto payloadResult = _readComponentValue(buf, *type);
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
        auto payloadResult = _readComponentValue(buf, *type);
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

Result<std::vector<u8>> readExactPredicateBytesFromWire(network::buffer::ByteBuf& buf)
{
    const usize start = buf.readPosition();
    i32 count = 0;
    MC_TRY_ASSIGN(count, buf.readVarInt());
    if (count < 0 || static_cast<usize>(count) > buf.readableBytes()) {
        return Error(ErrorCode::InvalidData, "Invalid component predicate count");
    }
    for (i32 i = 0; i < count; ++i) {
        i32 typeId = 0;
        MC_TRY_ASSIGN(typeId, buf.readVarInt());
        const auto type = componentTypeById(typeId);
        if (!type.has_value()) {
            return Error(ErrorCode::InvalidData, "Unknown component in trade predicate");
        }
        MC_TRY(_readComponentValue(buf, *type));
    }
    return std::vector<u8>(buf.data() + start, buf.data() + buf.readPosition());
}

std::pair<std::vector<std::pair<i32, i32>>, std::vector<i32>> computeComponentHashes(const DataComponentPatch& patch)
{
    std::pair<std::vector<std::pair<i32, i32>>, std::vector<i32>> result;
    for (const auto& entry : patch.added()) {
        auto type = componentTypeById(entry.typeId);
        if (!type.has_value()) {
            continue; // 未落地组件跳过
        }
        // 用专属 codec 把组件值编码到临时 ByteBuf，对 wire 字节算 CRC-32C。
        network::buffer::ByteBuf tmp;
        if (_writeComponentValue(tmp, *type, entry.value).failed()) {
            continue; // 编码失败跳过该组件（不中断整体哈希计算）
        }
        const u32 h = util::crypto::Crc32c::hash(std::span<const u8>(tmp.data(), tmp.size()));
        result.first.emplace_back(entry.typeId, static_cast<i32>(h));
    }
    for (i32 typeId : patch.removed()) {
        result.second.push_back(typeId);
    }
    return result;
}

} // namespace component
} // namespace item
} // namespace mc
