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

#include "common/network/codec/EntityMetadataSerializer.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <cstring>

namespace mc::network {

// ============================================================================
// 1.21.11 数据序列化器 ID（对齐 net.minecraft.network.syncher.EntityDataSerializers
// 静态初始化块的注册顺序——注册顺序即整数 ID，wire 上以 VarInt 传输）
// ============================================================================
namespace {
// 字符串长度上限（2^21 - 1，VarInt 3 字节能表示的最大值）。
// MC 协议字符串长度限制为 32767（Short.MAX_VALUE），但命令树等数据可能超过此限制；
// 使用 VarInt 编码可支持更大的字符串。本模块自用内联常量，与 PacketSerializer 的
// MAX_STRING_LENGTH 同值但独立定义，避免跨模块借常量。
constexpr u32 kMaxStringLength = 2097151u;

// MC 1.21.11 EntityDataSerializers 注册顺序（EntityDataSerializers.java:153-193）
enum class MetadataSerializerId : i32 {
    Byte = 0,              // i8              (ByteBufCodecs.BYTE)
    Int = 1,               // i32             (VAR_INT)
    Long = 2,              // i64             (VAR_LONG)
    Float = 3,             // f32             (FLOAT)
    String = 4,            // std::string     (STRING_UTF8)
    Component = 5,         // 文本组件（本项目暂用 String 承载）
    OptionalComponent = 6, // 可选文本组件
    ItemStack = 7,         // 物品堆
    Boolean = 8,           // bool            (BOOL)
    Rotations = 9,         // Vector3f / Vector2f (ROTATIONS，f32×3)
    BlockPos = 10,         // Vector3i        (BlockPos.STREAM_CODEC = i64 packed)
    OptionalBlockPos = 11, // 可选 BlockPos
    Direction = 12,        // 方向
    OptionalLivingEntityRef = 13,
    BlockState = 14,
    OptionalBlockState = 15,
    Particle = 16,
    Particles = 17,
    VillagerData = 18,
    OptionalUnsignedInt = 19, // OptionalInt
    Pose = 20,                // 姿态（本项目以 i8 承载枚举值，走 Byte）
};

// DataValue 变体索引 → 1.21.11 序列化器 ID
// 变体顺序（EntityDataManager::DataValue::ValueType）：
//   0:i8 1:i32 2:i64 3:f32 4:string 5:bool 6:Vector3i 7:Vector2f 8:Vector3f 9:ItemStackView
MetadataSerializerId getSerializerId(const entity::DataValue& value)
{
    switch (value.index()) {
        case 0:
            return MetadataSerializerId::Byte;
        case 1:
            return MetadataSerializerId::Int;
        case 2:
            return MetadataSerializerId::Long;
        case 3:
            return MetadataSerializerId::Float;
        case 4:
            return MetadataSerializerId::String;
        case 5:
            return MetadataSerializerId::Boolean;
        case 6:
            return MetadataSerializerId::BlockPos;
        case 7: // Vector2f → Rotations（z 补 0）
        case 8:
            return MetadataSerializerId::Rotations; // Vector3f → Rotations
        case 9:
            return MetadataSerializerId::ItemStack; // ItemStackView → ITEM_STACK（serializerId 7）
        default:
            return MetadataSerializerId::Byte;
    }
}
} // namespace

// ============================================================================
// 序列化
// ============================================================================

std::vector<u8> EntityMetadataSerializer::serialize(const entity::EntityDataManager& manager, bool dirtyOnly)
{
    std::vector<u8> output;

    const auto& entries = manager.getAllEntries();

    for (const auto& [id, entry] : entries) {
        // 如果只要脏数据，跳过非脏数据
        if (dirtyOnly && !entry.dirty) {
            continue;
        }

        serializeEntry(id, entry.value, output);
    }

    // 结束标记
    output.push_back(0xFF);

    return output;
}

void EntityMetadataSerializer::serializeEntry(u16 id, const entity::DataValue& value, std::vector<u8>& output)
{
    // 1.21.11 wire：byte(index) + VarInt(serializerId) + value
    // （对齐 SynchedEntityData.DataValue.write：writeByte(id) + writeVarInt(serializerId) + codec.encode）
    output.push_back(static_cast<u8>(id & 0xFF));
    _writeVarInt(static_cast<i32>(getSerializerId(value)), output);

    // 数据（按 1.21.11 序列化器 codec 编码）
    switch (value.index()) {
        case 0: { // i8 → BYTE
            output.push_back(static_cast<u8>(value.get<i8>()));
            break;
        }
        case 1: { // i32 → INT (VAR_INT)
            _writeVarInt(value.get<i32>(), output);
            break;
        }
        case 2: { // i64 → LONG (VAR_LONG)
            _writeVarLong(value.get<i64>(), output);
            break;
        }
        case 3: { // f32 → FLOAT（大端，Java friendly buf）
            f32 val = value.get<f32>();
            _writeBigEndianF32(val, output);
            break;
        }
        case 4: { // std::string → STRING (VarInt len + UTF-8)
            _writeString(value.get<std::string>(), output);
            break;
        }
        case 5: { // bool → BOOLEAN
            output.push_back(value.get<bool>() ? 1 : 0);
            break;
        }
        case 6: { // Vector3i → BLOCK_POS（i64 packed，X26/Z26/Y12，对齐 BlockPos.asLong）
            auto pos = value.get<Vector3i>();
            const i64 packed = BlockPos(pos.x, pos.y, pos.z).asLong();
            _writeBigEndianI64(packed, output);
            break;
        }
        case 7: { // Vector2f → ROTATIONS（f32×3，z 补 0）
            auto rot = value.get<Vector2f>();
            _writeBigEndianF32(rot.x, output);
            _writeBigEndianF32(rot.y, output);
            _writeBigEndianF32(0.0f, output);
            break;
        }
        case 8: { // Vector3f → ROTATIONS（f32×3）
            auto rot = value.get<Vector3f>();
            _writeBigEndianF32(rot.x, output);
            _writeBigEndianF32(rot.y, output);
            _writeBigEndianF32(rot.z, output);
            break;
        }
        case 9: { // ItemStackView → ITEM_STACK（1.21.11 OPTIONAL_STREAM_CODEC：
            // count<=0 即空（仅 VarInt(0)）；否则 VarInt(count) + VarInt(itemId)
            // + VarInt(componentsPatch.size()) + patch 字节。与 JavaPlayCodecs::writeItemStack 同源。）
            auto view = value.get<network::ir::play::ItemStackView>();
            if (view.count <= 0 || view.itemId == 0) {
                _writeVarInt(0, output);
            } else {
                _writeVarInt(view.count, output);
                _writeVarInt(static_cast<i32>(view.itemId), output);
                _writeVarInt(static_cast<i32>(view.componentsPatch.size()), output);
                output.insert(output.end(), view.componentsPatch.begin(), view.componentsPatch.end());
            }
            break;
        }
        default:
            // 未知类型：写空字节占位（不应发生——getSerializerId 已兜底 Byte）
            output.push_back(0);
            break;
    }
}

// ============================================================================
// 反序列化
// ============================================================================

bool EntityMetadataSerializer::deserialize(const std::vector<u8>& data, entity::EntityDataManager& manager)
{
    size_t offset = 0;

    while (offset < data.size()) {
        // 1.21.11 wire：byte(index) + VarInt(serializerId) + value，0xFF 结束
        u8 index = data[offset++];

        // 检查结束标记
        if (index == 0xFF) {
            break;
        }

        // 序列化器 ID（VarInt，对齐 DataValue.read）
        i32 serializerId = _readVarInt(data.data(), data.size(), offset);
        const auto sid = static_cast<MetadataSerializerId>(serializerId);

        switch (sid) {
            case MetadataSerializerId::Byte: {
                if (offset >= data.size()) return false;
                const i8 value = static_cast<i8>(data[offset++]);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Int: {
                i32 value = _readVarInt(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Long: {
                i64 value = _readVarLong(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Float: {
                f32 val;
                if (!_readBigEndianF32(data.data(), data.size(), offset, val)) return false;
                (void)manager.setRaw(index, entity::DataValue(val));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::String:
            case MetadataSerializerId::Component:
            case MetadataSerializerId::OptionalComponent: {
                // Component 暂以字符串承载。1.21.11 真组件 NBT codec 依赖 ITextComponent 的 NBT
                // 序列化（对齐 ComponentSerialization.CODEC），属独立大项；落地后此处改读 NBT compound。
                std::string value = _readString(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Boolean: {
                if (offset >= data.size()) return false;
                bool value = data[offset++] != 0;
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::BlockPos: {
                i64 packed;
                if (!_readBigEndianI64(data.data(), data.size(), offset, packed)) return false;
                const BlockPos pos = BlockPos::fromLong(packed);
                (void)manager.setRaw(index, entity::DataValue(entity::Vector3i(pos.x, pos.y, pos.z)));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Rotations: {
                // 1.21.11 ROTATIONS = f32×3。本项目 DataValue 无独立 Rotations 类型，
                // 反序列化为 Vector3f（覆盖 x/y/z）。Vector2f 写入端补 z=0，读端仍得 Vector3f——
                // 因 Vector2f/Vector3f 当前无活元数据参数，此对称差异不影响线上行为。
                f32 x;
                f32 y;
                f32 z;
                if (!_readBigEndianF32(data.data(), data.size(), offset, x)) return false;
                if (!_readBigEndianF32(data.data(), data.size(), offset, y)) return false;
                if (!_readBigEndianF32(data.data(), data.size(), offset, z)) return false;
                (void)manager.setRaw(index, entity::DataValue(entity::Vector3f(x, y, z)));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::ItemStack: {
                // 1.21.11 ITEM_STACK = OPTIONAL_STREAM_CODEC：VarInt(count)；count<=0 即空；
                // 否则 VarInt(itemId) + VarInt(patchLen) + patchLen 字节。patch 是
                // DataComponentPatch 的 1.21.11 wire 字节，元数据层透传存入 ItemStackView，
                // 由客户端 fromItemStackView 还原为业务侧 ItemStack。
                const i32 count = _readVarInt(data.data(), data.size(), offset);
                network::ir::play::ItemStackView view;
                if (count <= 0) {
                    view.itemId = 0;
                    view.count = 0;
                } else {
                    const i32 itemId = _readVarInt(data.data(), data.size(), offset);
                    const i32 patchLen = _readVarInt(data.data(), data.size(), offset);
                    if (patchLen < 0 || offset + static_cast<size_t>(patchLen) > data.size()) {
                        return false;
                    }
                    view.itemId = static_cast<u32>(itemId);
                    view.count = count;
                    view.componentsPatch.assign(data.data() + offset, data.data() + offset + patchLen);
                    offset += static_cast<size_t>(patchLen);
                }
                (void)manager.setRaw(index, entity::DataValue(view));
                manager.clearDirty(index);
                break;
            }
            default:
                // 未实现的序列化器（Particle/VillagerData 等）：无法跳过变长值，停止解析。
                // 本项目元数据仅用 Byte/Int/Long/Float/String/Boolean/ItemStack，故正常路径不会落到此分支。
                return false;
        }
    }

    return true;
}

// ============================================================================
// 辅助方法
// ============================================================================

void EntityMetadataSerializer::_writeVarInt(i32 value, std::vector<u8>& output) noexcept
{
    u32 uval = static_cast<u32>(value);
    do {
        u8 byte = uval & 0x7F;
        uval >>= 7;
        if (uval != 0) {
            byte |= 0x80;
        }
        output.push_back(byte);
    } while (uval != 0);
}

void EntityMetadataSerializer::_writeVarLong(i64 value, std::vector<u8>& output) noexcept
{
    u64 uval = static_cast<u64>(value);
    do {
        u8 byte = uval & 0x7F;
        uval >>= 7;
        if (uval != 0) {
            byte |= 0x80;
        }
        output.push_back(byte);
    } while (uval != 0);
}

i32 EntityMetadataSerializer::_readVarInt(const u8* data, size_t size, size_t& offset) noexcept
{
    i32 result = 0;
    i32 shift = 0;

    while (offset < size) {
        u8 byte = data[offset++];
        result |= static_cast<i32>(byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            break;
        }
    }

    return result;
}

i64 EntityMetadataSerializer::_readVarLong(const u8* data, size_t size, size_t& offset) noexcept
{
    i64 result = 0;
    i32 shift = 0;

    while (offset < size) {
        u8 byte = data[offset++];
        result |= static_cast<i64>(byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            break;
        }
    }

    return result;
}

void EntityMetadataSerializer::_writeString(const std::string& str, std::vector<u8>& output) noexcept
{
    // 截断过长的字符串
    size_t writeLen = std::min(str.size(), static_cast<size_t>(kMaxStringLength));
    // 字符串长度 (VarInt)
    _writeVarInt(static_cast<i32>(writeLen), output);
    // 字符串内容
    output.insert(output.end(), str.begin(), str.begin() + writeLen);
}

std::string EntityMetadataSerializer::_readString(const u8* data, size_t size, size_t& offset) noexcept
{
    i32 length = _readVarInt(data, size, offset);

    // 验证长度
    if (length < 0) {
        return "";
    }
    if (static_cast<size_t>(length) > kMaxStringLength) {
        return ""; // 字符串过长，返回空字符串
    }
    if (offset + static_cast<size_t>(length) > size) {
        return "";
    }

    std::string result(reinterpret_cast<const char*>(data + offset), static_cast<size_t>(length));
    offset += static_cast<size_t>(length);
    return result;
}

void EntityMetadataSerializer::_writeBigEndianF32(f32 value, std::vector<u8>& output) noexcept
{
    // Java FriendlyByteBuf.writeFloat 大端；按字节序无关方式拼装
    u32 bits;
    std::memcpy(&bits, &value, sizeof(f32));
    output.push_back(static_cast<u8>((bits >> 24) & 0xFF));
    output.push_back(static_cast<u8>((bits >> 16) & 0xFF));
    output.push_back(static_cast<u8>((bits >> 8) & 0xFF));
    output.push_back(static_cast<u8>(bits & 0xFF));
}

void EntityMetadataSerializer::_writeBigEndianI64(i64 value, std::vector<u8>& output) noexcept
{
    u64 bits = static_cast<u64>(value);
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.push_back(static_cast<u8>((bits >> shift) & 0xFF));
    }
}

bool EntityMetadataSerializer::_readBigEndianF32(const u8* data, size_t size, size_t& offset, f32& out) noexcept
{
    if (offset + sizeof(f32) > size) return false;
    u32 bits = (static_cast<u32>(data[offset]) << 24) | (static_cast<u32>(data[offset + 1]) << 16) |
        (static_cast<u32>(data[offset + 2]) << 8) | static_cast<u32>(data[offset + 3]);
    std::memcpy(&out, &bits, sizeof(f32));
    offset += sizeof(f32);
    return true;
}

bool EntityMetadataSerializer::_readBigEndianI64(const u8* data, size_t size, size_t& offset, i64& out) noexcept
{
    if (offset + sizeof(i64) > size) return false;
    u64 bits = 0;
    for (int i = 0; i < 8; ++i) {
        bits = (bits << 8) | static_cast<u64>(data[offset + i]);
    }
    out = static_cast<i64>(bits);
    offset += sizeof(i64);
    return true;
}

} // namespace mc::network
