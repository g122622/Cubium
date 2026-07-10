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
#include "common/world/block/BlockPos.hpp"
#include <array>
#include <string>

namespace mc::network {

// ============================================================================
// 打开告示牌编辑器包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 打开告示牌编辑器包
 *
 * 服务端通知客户端打开指定位置的告示牌编辑界面。
 * 客户端收到后弹出 AbstractSignEditScreen，加载该告示牌的当前文本，
 * 允许玩家编辑4行文本。
 *
 * 协议格式: x(i32) + y(i32) + z(i32) + isFrontSide(bool)
 */
class OpenSignEditorPacket {
public:
    OpenSignEditorPacket() = default;

    /**
     * @brief 构造打开告示牌编辑器包
     * @param pos 告示牌方块位置
     * @param isFrontSide 是否编辑正面（false=背面）
     */
    OpenSignEditorPacket(const BlockPos& pos, bool isFrontSide)
        : m_pos(pos)
        , m_isFrontSide(isFrontSide)
    {}

    // Getters
    [[nodiscard]] const BlockPos& pos() const { return m_pos; }
    [[nodiscard]] bool isFrontSide() const { return m_isFrontSide; }

    // Setters
    void setPos(const BlockPos& pos) { m_pos = pos; }
    void setIsFrontSide(bool isFrontSide) { m_isFrontSide = isFrontSide; }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeI32(m_pos.x);
        ser.writeI32(m_pos.y);
        ser.writeI32(m_pos.z);
        ser.writeBool(m_isFrontSide);
    }

    // 反序列化
    [[nodiscard]] static Result<OpenSignEditorPacket> deserialize(PacketDeserializer& deser)
    {
        OpenSignEditorPacket packet;

        auto xResult = deser.readI32();
        if (xResult.failed()) return xResult.error();
        packet.m_pos.x = xResult.value();

        auto yResult = deser.readI32();
        if (yResult.failed()) return yResult.error();
        packet.m_pos.y = yResult.value();

        auto zResult = deser.readI32();
        if (zResult.failed()) return zResult.error();
        packet.m_pos.z = zResult.value();

        auto frontResult = deser.readBool();
        if (frontResult.failed()) return frontResult.error();
        packet.m_isFrontSide = frontResult.value();

        return packet;
    }

private:
    BlockPos m_pos;            ///< 告示牌方块位置
    bool m_isFrontSide = true; ///< 是否编辑正面
};

// ============================================================================
// 告示牌文本更新包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 告示牌文本更新包
 *
 * 客户端在告示牌编辑器关闭时，将编辑后的4行文本发送给服务端。
 * 服务端收到后更新对应告示牌方块实体的文本，并清除编辑锁。
 *
 * 协议格式: x(i32) + y(i32) + z(i32) + 4行文本(各string) + isFrontSide(bool)
 */
class UpdateSignPacket {
public:
    /// 告示牌行数
    static constexpr i32 LINE_COUNT = 4;

    UpdateSignPacket() = default;

    /**
     * @brief 构造告示牌文本更新包
     * @param pos 告示牌方块位置
     * @param lines 4行文本
     * @param isFrontSide 是否编辑正面
     */
    UpdateSignPacket(const BlockPos& pos, const std::array<std::string, LINE_COUNT>& lines, bool isFrontSide)
        : m_pos(pos)
        , m_lines(lines)
        , m_isFrontSide(isFrontSide)
    {}

    // Getters
    [[nodiscard]] const BlockPos& pos() const { return m_pos; }
    [[nodiscard]] const std::array<std::string, LINE_COUNT>& lines() const { return m_lines; }
    [[nodiscard]] bool isFrontSide() const { return m_isFrontSide; }

    // Setters
    void setPos(const BlockPos& pos) { m_pos = pos; }
    void setLines(const std::array<std::string, LINE_COUNT>& lines) { m_lines = lines; }
    void setIsFrontSide(bool isFrontSide) { m_isFrontSide = isFrontSide; }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeI32(m_pos.x);
        ser.writeI32(m_pos.y);
        ser.writeI32(m_pos.z);
        for (const auto& line : m_lines) {
            ser.writeString(line);
        }
        ser.writeBool(m_isFrontSide);
    }

    // 反序列化
    [[nodiscard]] static Result<UpdateSignPacket> deserialize(PacketDeserializer& deser)
    {
        UpdateSignPacket packet;

        auto xResult = deser.readI32();
        if (xResult.failed()) return xResult.error();
        packet.m_pos.x = xResult.value();

        auto yResult = deser.readI32();
        if (yResult.failed()) return yResult.error();
        packet.m_pos.y = yResult.value();

        auto zResult = deser.readI32();
        if (zResult.failed()) return zResult.error();
        packet.m_pos.z = zResult.value();

        for (i32 i = 0; i < LINE_COUNT; ++i) {
            auto lineResult = deser.readString();
            if (lineResult.failed()) return lineResult.error();
            packet.m_lines[static_cast<std::size_t>(i)] = lineResult.value();
        }

        auto frontResult = deser.readBool();
        if (frontResult.failed()) return frontResult.error();
        packet.m_isFrontSide = frontResult.value();

        return packet;
    }

private:
    BlockPos m_pos;                              ///< 告示牌方块位置
    std::array<std::string, LINE_COUNT> m_lines; ///< 4行文本
    bool m_isFrontSide = true;                   ///< 是否编辑正面
};

} // namespace mc::network
