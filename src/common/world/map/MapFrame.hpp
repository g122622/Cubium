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

#include "core/Types.hpp"
#include "util/nbt/Nbt.hpp"
#include "world/block/BlockPos.hpp"
#include <string>

namespace mc::world::map {

/**
 * @brief 地图展示框标记
 *
 * 记录地图在物品展示框中的位置和旋转信息，用于在地图上显示绿色标记点。
 */
class MapFrame {
public:
    MapFrame() = default;
    MapFrame(BlockPos pos, i32 rotation, i32 entityId) noexcept;

    /**
     * @brief 从NBT读取展示框标记
     */
    static MapFrame fromNbt(const nbt::tags::compound_tag& tag);

    /**
     * @brief 写入NBT
     */
    void toNbt(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 获取唯一标识字符串
     */
    [[nodiscard]] std::string getId() const;

    /**
     * @brief 根据方块位置生成ID
     */
    [[nodiscard]] static std::string getIdForPos(BlockPos pos);

    [[nodiscard]] BlockPos pos() const noexcept { return m_pos; }
    [[nodiscard]] i32 rotation() const noexcept { return m_rotation; }
    [[nodiscard]] i32 entityId() const noexcept { return m_entityId; }

private:
    BlockPos m_pos;
    i32 m_rotation; // 0-3，对应0°/90°/180°/270°
    i32 m_entityId;
};

} // namespace mc::world::map
