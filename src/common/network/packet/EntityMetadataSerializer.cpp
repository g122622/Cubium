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

#include "EntityMetadataSerializer.hpp"
#include "../../util/math/Vector3.hpp"
#include "PacketSerializer.hpp"
#include <cstring>

namespace mc::network {

// ============================================================================
// 类型ID映射
// ============================================================================

namespace {
// MC 1.16.5 数据类型ID
enum class MetadataTypeId : u8 {
    Byte = 0,
    VarInt = 1,
    Float = 2,
    String = 3,
    TextComponent = 4,
    OptChat = 5,
    Slot = 6,
    Boolean = 7,
    Rotation = 8,
    Position = 9,
    OptPosition = 10,
    Direction = 11,
    OptUUID = 12,
    OptBlockID = 13,
    NBT = 14,
    Particle = 15,
    VillagerData = 16,
    OptVarInt = 17,
    Pose = 18
};

// DataValue 索引到类型ID映射
MetadataTypeId getTypeId(const entity::DataValue& value)
{
    switch (value.index()) {
        case 0: // i8
            return MetadataTypeId::Byte;
        case 1: // i32
            return MetadataTypeId::VarInt;
        case 2:                            // i64
            return MetadataTypeId::VarInt; // 使用 VarInt 近似
        case 3:                            // f32
            return MetadataTypeId::Float;
        case 4: // std::string
            return MetadataTypeId::String;
        case 5: // bool
            return MetadataTypeId::Boolean;
        case 6: // Vector3i
            return MetadataTypeId::Position;
        case 7: // Vector2f
            return MetadataTypeId::Rotation;
        case 8: // Vector3f
            return MetadataTypeId::Rotation;
        default:
            return MetadataTypeId::Byte;
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
    // 参数ID (u8，因为MC使用单字节索引)
    output.push_back(static_cast<u8>(id & 0xFF));

    // 类型ID
    MetadataTypeId typeId = getTypeId(value);
    output.push_back(static_cast<u8>(typeId));

    // 数据
    switch (value.index()) {
        case 0: { // i8
            output.push_back(static_cast<u8>(value.get<i8>()));
            break;
        }
        case 1: { // i32
            writeVarInt(value.get<i32>(), output);
            break;
        }
        case 2: { // i64
            writeVarLong(value.get<i64>(), output);
            break;
        }
        case 3: { // f32
            f32 val = value.get<f32>();
            u8* bytes = reinterpret_cast<u8*>(&val);
            for (size_t i = 0; i < sizeof(f32); ++i) {
                output.push_back(bytes[i]);
            }
            break;
        }
        case 4: { // std::string
            writeString(value.get<std::string>(), output);
            break;
        }
        case 5: { // bool
            output.push_back(value.get<bool>() ? 1 : 0);
            break;
        }
        case 6: { // Vector3i (BlockPos)
            auto pos = value.get<Vector3i>();
            // BlockPos 编码为 VarInt (x, y, z)
            writeVarInt(pos.x, output);
            writeVarInt(pos.y, output);
            writeVarInt(pos.z, output);
            break;
        }
        case 7: { // Vector2f (Rotation - pitch, yaw)
            auto rot = value.get<Vector2f>();
            u8* bytesX = reinterpret_cast<u8*>(&rot.x);
            u8* bytesY = reinterpret_cast<u8*>(&rot.y);
            for (size_t i = 0; i < sizeof(f32); ++i) {
                output.push_back(bytesX[i]);
            }
            for (size_t i = 0; i < sizeof(f32); ++i) {
                output.push_back(bytesY[i]);
            }
            break;
        }
        case 8: { // Vector3f (Rotation - x, y, z)
            auto rot = value.get<Vector3f>();
            u8* bytesX = reinterpret_cast<u8*>(&rot.x);
            u8* bytesY = reinterpret_cast<u8*>(&rot.y);
            u8* bytesZ = reinterpret_cast<u8*>(&rot.z);
            for (size_t i = 0; i < sizeof(f32); ++i) {
                output.push_back(bytesX[i]);
            }
            for (size_t i = 0; i < sizeof(f32); ++i) {
                output.push_back(bytesY[i]);
            }
            for (size_t i = 0; i < sizeof(f32); ++i) {
                output.push_back(bytesZ[i]);
            }
            break;
        }
        default:
            // 未知类型，写入0
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
        u8 index = data[offset++];

        // 检查结束标记
        if (index == 0xFF) {
            break;
        }

        if (offset >= data.size()) {
            return false; // 数据不完整
        }

        u8 typeId = data[offset++];

        switch (static_cast<MetadataTypeId>(typeId)) {
            case MetadataTypeId::Byte: {
                if (offset >= data.size()) return false;
                const i8 value = static_cast<i8>(data[offset++]);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataTypeId::VarInt: {
                i32 value = readVarInt(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataTypeId::Float: {
                if (offset + sizeof(f32) > data.size()) return false;
                f32 val;
                std::memcpy(&val, data.data() + offset, sizeof(f32));
                offset += sizeof(f32);
                (void)manager.setRaw(index, entity::DataValue(val));
                manager.clearDirty(index);
                break;
            }
            case MetadataTypeId::String: {
                std::string value = readString(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataTypeId::Boolean: {
                if (offset >= data.size()) return false;
                bool value = data[offset++] != 0;
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataTypeId::Position: {
                i32 x = readVarInt(data.data(), data.size(), offset);
                i32 y = readVarInt(data.data(), data.size(), offset);
                i32 z = readVarInt(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(entity::Vector3i(x, y, z)));
                manager.clearDirty(index);
                break;
            }
            case MetadataTypeId::Rotation: {
                if (offset + sizeof(f32) * 2 > data.size()) return false;
                f32 x;
                f32 y;
                std::memcpy(&x, data.data() + offset, sizeof(f32));
                offset += sizeof(f32);
                std::memcpy(&y, data.data() + offset, sizeof(f32));
                offset += sizeof(f32);
                (void)manager.setRaw(index, entity::DataValue(entity::Vector2f(x, y)));
                manager.clearDirty(index);
                break;
            }
            default:
                // 未知类型，无法继续解析
                return false;
        }
    }

    return true;
}

// ============================================================================
// 辅助方法
// ============================================================================

void EntityMetadataSerializer::writeVarInt(i32 value, std::vector<u8>& output)
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

void EntityMetadataSerializer::writeVarLong(i64 value, std::vector<u8>& output)
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

i32 EntityMetadataSerializer::readVarInt(const u8* data, size_t size, size_t& offset)
{
    i32 result = 0;
    int shift = 0;

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

i64 EntityMetadataSerializer::readVarLong(const u8* data, size_t size, size_t& offset)
{
    i64 result = 0;
    int shift = 0;

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

void EntityMetadataSerializer::writeString(const std::string& str, std::vector<u8>& output)
{
    // 截断过长的字符串
    size_t writeLen = std::min(str.size(), static_cast<size_t>(MAX_STRING_LENGTH));
    // 字符串长度 (VarInt)
    writeVarInt(static_cast<i32>(writeLen), output);
    // 字符串内容
    output.insert(output.end(), str.begin(), str.begin() + writeLen);
}

std::string EntityMetadataSerializer::readString(const u8* data, size_t size, size_t& offset)
{
    i32 length = readVarInt(data, size, offset);

    // 验证长度
    if (length < 0) {
        return "";
    }
    if (static_cast<size_t>(length) > MAX_STRING_LENGTH) {
        return ""; // 字符串过长，返回空字符串
    }
    if (offset + static_cast<size_t>(length) > size) {
        return "";
    }

    std::string result(reinterpret_cast<const char*>(data + offset), static_cast<size_t>(length));
    offset += static_cast<size_t>(length);
    return result;
}

} // namespace mc::network
