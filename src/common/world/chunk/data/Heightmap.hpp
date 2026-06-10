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

#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include <array>

// BlockState 在 mc 命名空间中定义
namespace mc {
class BlockState;
} // namespace mc

namespace mc::world::chunk {

// ============================================================================
// 区块高度图类型
// ============================================================================

enum class HeightmapType : u8 {
    WorldSurface,           // 最高非空气方块
    OceanFloor,             // 最高固体方块
    MotionBlocking,         // 最高阻挡运动方块
    MotionBlockingNoLeaves, // 最高阻挡运动方块（不含树叶）
    WorldSurfaceWG,         // 世界表面（生成时）
    OceanFloorWG,           // 海底（生成时）
    LightBlocking           // 最高阻挡光照方块
};

// ============================================================================
// 高度图
// ============================================================================

/**
 * @brief 高度图
 *
 * 存储每个 XZ 位置的最高方块 Y 坐标。
 */
class Heightmap {
public:
    static constexpr i32 SIZE = mc::world::CHUNK_WIDTH * mc::world::CHUNK_WIDTH;

    explicit Heightmap(HeightmapType type = HeightmapType::WorldSurface);

    /**
     * @brief 更新高度图
     * @param x 区块内 X 坐标 (0-15)
     * @param y 方块 Y 坐标
     * @param z 区块内 Z 坐标 (0-15)
     * @param state 方块状态
     * @return true 如果高度更新
     */
    bool update(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state);

    /**
     * @brief 获取高度
     */
    [[nodiscard]] BlockCoord getHeight(BlockCoord x, BlockCoord z) const;

    /**
     * @brief 设置高度数据（从存档加载）
     */
    void setData(const std::array<BlockCoord, SIZE>& data);

    /**
     * @brief 将所有高度值设为指定值
     */
    void setAll(BlockCoord value) { m_heights.fill(value); }

    /**
     * @brief 获取高度数据
     */
    [[nodiscard]] const std::array<BlockCoord, SIZE>& getData() const { return m_heights; }

    [[nodiscard]] HeightmapType getType() const { return m_type; }

private:
    HeightmapType m_type;
    std::array<BlockCoord, SIZE> m_heights;

    /**
     * @brief 检查方块是否影响此高度图
     */
    [[nodiscard]] bool _isOpaque(const BlockState* state) const;
};

} // namespace mc::world::chunk

namespace mc {
using HeightmapType = mc::world::chunk::HeightmapType;
using Heightmap = mc::world::chunk::Heightmap;
} // namespace mc
