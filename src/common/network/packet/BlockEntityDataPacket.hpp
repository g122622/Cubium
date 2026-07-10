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

#include "PacketDeserializer.hpp"
#include "PacketSerializer.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mc::network {

/**
 * @brief 方块实体数据同步包 (S->C)
 *
 * 服务端在方块实体数据发生变化（如告示牌文本更新、箱子内容变化等）时，
 * 将最新的方块实体 NBT 数据广播给追踪该区块的客户端。
 *
 * 客户端收到后根据 BlockEntityType 在 ClientWorld 中查找或创建对应的
 * BlockEntity 实例，并通过 loadFromNBT() 更新其状态。
 *
 * 参考 MC Java:
 *   net.minecraft.network.protocol.game.ClientboundBlockEntityDataPacket
 *
 * 协议格式:
 *   x(i32) + y(i32) + z(i32) + type(u16) + nbtBytesLen(varuint) + nbtBytes(bytes)
 *
 * NBT 字节流采用 Java 版大端序二进制格式（nbt::Contexts::java），
 * 复合标签为可能包含 id/x/y/z 等公共字段及类型自定义字段的完整快照。
 */
class BlockEntityDataPacket {
public:
    BlockEntityDataPacket() = default;

    /**
     * @brief 构造方块实体数据同步包
     * @param pos 方块位置
     * @param type 方块实体类型
     * @param nbtData NBT 字节流（Java 版大端序二进制格式）
     */
    BlockEntityDataPacket(const BlockPos& pos, BlockEntityType type, std::vector<u8> nbtData)
        : m_pos(pos)
        , m_type(type)
        , m_nbtData(std::move(nbtData))
    {}

    /**
     * @brief 构造方块实体数据同步包（便捷构造，自动序列化 NBT）
     * @param pos 方块位置
     * @param type 方块实体类型
     * @param tag NBT 复合标签（将按 Java 版格式序列化为字节流）
     */
    BlockEntityDataPacket(const BlockPos& pos, BlockEntityType type, const nbt::CompoundTag& tag)
        : m_pos(pos)
        , m_type(type)
    {
        m_nbtData = serializeNbtToBytes(tag);
    }

    // Getters
    [[nodiscard]] const BlockPos& pos() const { return m_pos; }
    [[nodiscard]] BlockEntityType type() const { return m_type; }
    [[nodiscard]] const std::vector<u8>& nbtData() const { return m_nbtData; }

    // Setters
    void setPos(const BlockPos& pos) { m_pos = pos; }
    void setType(BlockEntityType type) { m_type = type; }
    void setNbtData(std::vector<u8> data) { m_nbtData = std::move(data); }

    /**
     * @brief 将内部 NBT 字节流反序列化为复合标签
     * @return 反序列化后的复合标签；失败返回错误
     *
     * 客户端处理此包时调用本方法获取方块实体状态数据，
     * 再传给 BlockEntity::loadFromNBT()。
     */
    [[nodiscard]] Result<nbt::CompoundTag> parseNbt() const { return deserializeNbtFromBytes(m_nbtData); }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeI32(m_pos.x);
        ser.writeI32(m_pos.y);
        ser.writeI32(m_pos.z);
        ser.writeU16(static_cast<u16>(m_type));
        ser.writeVarUInt(static_cast<u32>(m_nbtData.size()));
        if (!m_nbtData.empty()) {
            ser.writeBytes(m_nbtData.data(), m_nbtData.size());
        }
    }

    // 反序列化
    [[nodiscard]] static Result<BlockEntityDataPacket> deserialize(PacketDeserializer& deser)
    {
        BlockEntityDataPacket packet;

        auto xResult = deser.readI32();
        if (xResult.failed()) return xResult.error();
        packet.m_pos.x = xResult.value();

        auto yResult = deser.readI32();
        if (yResult.failed()) return yResult.error();
        packet.m_pos.y = yResult.value();

        auto zResult = deser.readI32();
        if (zResult.failed()) return zResult.error();
        packet.m_pos.z = zResult.value();

        auto typeResult = deser.readU16();
        if (typeResult.failed()) return typeResult.error();
        packet.m_type = static_cast<BlockEntityType>(typeResult.value());

        auto lenResult = deser.readVarUInt();
        if (lenResult.failed()) return lenResult.error();
        const u32 nbtLen = lenResult.value();

        if (nbtLen > 0) {
            if (!deser.hasRemaining(nbtLen)) {
                return Error(ErrorCode::InvalidData, "BlockEntityDataPacket: NBT bytes exceed packet bounds");
            }
            auto bytesResult = deser.readBytes(nbtLen);
            if (bytesResult.failed()) return bytesResult.error();
            packet.m_nbtData = std::move(bytesResult.value());
        }

        return packet;
    }

    /**
     * @brief 将 NBT 复合标签序列化为 Java 版二进制字节流
     * @param tag NBT 复合标签
     * @return 字节流；失败时返回空向量（视为无 NBT 数据）
     *
     * 参考 ScoreboardSaveData::serializeNbtToBytes 的实现模式。
     */
    static std::vector<u8> serializeNbtToBytes(const nbt::CompoundTag& tag)
    {
        try {
            std::ostringstream oss(std::ios::binary);
            oss << nbt::Contexts::java;
            nbt::operator<<(oss, tag);
            if (!oss) {
                return {};
            }
            std::string str = oss.str();
            return std::vector<u8>(str.begin(), str.end());
        }
        catch (const std::exception&) {
            return {};
        }
    }

    /**
     * @brief 将 Java 版二进制字节流反序列化为 NBT 复合标签
     * @param data 字节流
     * @return 复合标签；失败返回错误
     */
    static Result<nbt::CompoundTag> deserializeNbtFromBytes(const std::vector<u8>& data)
    {
        if (data.empty()) {
            return Error(ErrorCode::InvalidData, "BlockEntityDataPacket: empty NBT data");
        }
        try {
            std::istringstream iss(std::string(data.begin(), data.end()), std::ios::binary);
            iss >> nbt::Contexts::java;
            auto root = nbt::tags::compound_tag::read(iss);
            if (!root) {
                return Error(ErrorCode::InvalidData, "BlockEntityDataPacket: failed to read NBT");
            }
            return std::move(*root);
        }
        catch (const std::exception& e) {
            return Error(ErrorCode::InvalidData, std::string("BlockEntityDataPacket: NBT parse error: ") + e.what());
        }
    }

private:
    BlockPos m_pos;                                    ///< 方块位置
    BlockEntityType m_type = BlockEntityType::Unknown; ///< 方块实体类型
    std::vector<u8> m_nbtData;                         ///< NBT 字节流（Java 版大端序二进制格式）
};

} // namespace mc::network
