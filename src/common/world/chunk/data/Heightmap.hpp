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
#include <optional>
#include <string>

// BlockState / Block 在 mc 命名空间中定义
namespace mc {
class BlockState;
class Block;
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
    LightBlocking,          // 最高阻挡光照方块
    COUNT                   // 高度图类型总数（用于数组索引上界）
};

// 高度图类型数量（编译期常量，用于 std::array 索引）
constexpr size_t HEIGHTMAP_TYPE_COUNT = static_cast<size_t>(HeightmapType::COUNT);

// ============================================================================
// 高度图
// ============================================================================

/**
 * @brief 高度图
 *
 * 存储每个 XZ 位置的最高方块 Y 坐标。
 *
 * 内部存储语义：每个槽位存储"最高方块 Y+1"（即上方空气方块位置）。
 * "无方块"列使用 NO_BLOCK_SENTINEL 标记。
 *
 * 历史上曾使用 0 作为"无方块"哨兵，但这与 Y=-1 的 Y+1=0 冲突，
 * 在主世界（minY=-64）等支持负 Y 的维度中无法区分"无方块"与"Y=-1 处有方块"。
 * 现使用 MIN_BUILD_HEIGHT - 1（主世界为 -65）作为哨兵，确保任何合法 Y+1
 * （范围 [minY+1, maxY] = [-63, 320]）都不会与哨兵冲突。
 */
class Heightmap {
public:
    static constexpr i32 SIZE = mc::world::CHUNK_WIDTH * mc::world::CHUNK_WIDTH;

    /**
     * @brief "无方块"哨兵值
     *
     * 取 MIN_BUILD_HEIGHT - 1（主世界为 -65），低于任何合法方块的 Y+1，
     * 因此不会与真实高度混淆。getHeight 返回此值表示该列无任何阻挡方块。
     */
    static constexpr BlockCoord NO_BLOCK_SENTINEL = mc::world::MIN_BUILD_HEIGHT - 1;

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
     *
     * @return 最高方块 Y+1，或 NO_BLOCK_SENTINEL 表示该列无方块
     */
    [[nodiscard]] BlockCoord getHeight(BlockCoord x, BlockCoord z) const;

    /**
     * @brief 直接设置指定 XZ 位置的高度（绕过 _isOpaque 判定，用于整列重算或从存档恢复）
     * @param x 区块内 X 坐标 (0-15)
     * @param z 区块内 Z 坐标 (0-15)
     * @param height 高度值（Heightmap 内部存储语义，即最高方块 Y+1，NO_BLOCK_SENTINEL 表示无方块）
     */
    void setHeight(BlockCoord x, BlockCoord z, BlockCoord height);

    /**
     * @brief 设置高度数据（从存档加载）
     *
     * @param data 高度数据数组，元素语义同 setHeight（Y+1 或 NO_BLOCK_SENTINEL）
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

    /**
     * @brief 按高度图类型判定方块是否计入该高度图
     *
     * 高度图判定逻辑的唯一权威入口。供 SpawnLocationHelper、NoiseChunkGenerator 等
     * 外部复用，避免判定逻辑多处复制后漂移（历史上 Heightmap::_isOpaque、
     * SpawnLocationHelper::_matchesHeightmap、NoiseChunkGenerator 内联 lambda 三处
     * 复制曾因各自演化而语义不一致）。
     *
     * @param type 高度图类型
     * @param state 方块状态（可为 nullptr，返回 false）
     */
    [[nodiscard]] static bool isOpaqueForType(HeightmapType type, const BlockState* state);

private:
    HeightmapType m_type;
    std::array<BlockCoord, SIZE> m_heights;

    /**
     * @brief 检查方块是否影响此高度图
     */
    [[nodiscard]] bool _isOpaque(const BlockState* state) const;

    /**
     * @brief 近似原版 blocksMotion() = isSolid() && block != COBWEB
     *
     * 项目无 blocksMotion() 方法，用 isSolid + Block 指针排除蜘蛛网近似。
     * bamboo_sapling 用 REPLACEABLE_PLANT（isSolid=false）天然不命中，无需特判。
     */
    [[nodiscard]] static bool _blocksMotion(const Block& block, const BlockState& state);

    /**
     * @brief 是否非树叶方块（原版 !(block instanceof LeavesBlock)）
     *
     * 项目树叶 isSolid=false 已被上层 _blocksMotion 过滤，但 NoLeaves 仍需显式排除。
     * 按 Material::LEAVES 指针比较（轻量，无需 RTTI）。
     */
    [[nodiscard]] static bool _isNotLeaf(const Block& block);
};

/**
 * @brief 把 MC 高度图序列化名（全大写）解析为 HeightmapType
 *
 * MC 1.21.11 Heightmap.Types 的 6 个合法序列化名：WORLD_SURFACE_WG / WORLD_SURFACE /
 * OCEAN_FLOOR_WG / OCEAN_FLOOR / MOTION_BLOCKING / MOTION_BLOCKING_NO_LEAVES。
 * 大小写敏感（MC 序列化名恒为全大写）。未知名返回 nullopt。
 */
[[nodiscard]] inline std::optional<HeightmapType> heightmapTypeFromString(const std::string& name)
{
    if (name == "WORLD_SURFACE_WG") {
        return HeightmapType::WorldSurfaceWG;
    }
    if (name == "WORLD_SURFACE") {
        return HeightmapType::WorldSurface;
    }
    if (name == "OCEAN_FLOOR_WG") {
        return HeightmapType::OceanFloorWG;
    }
    if (name == "OCEAN_FLOOR") {
        return HeightmapType::OceanFloor;
    }
    if (name == "MOTION_BLOCKING") {
        return HeightmapType::MotionBlocking;
    }
    if (name == "MOTION_BLOCKING_NO_LEAVES") {
        return HeightmapType::MotionBlockingNoLeaves;
    }
    return std::nullopt;
}

} // namespace mc::world::chunk

namespace mc {
using HeightmapType = mc::world::chunk::HeightmapType;
using Heightmap = mc::world::chunk::Heightmap;
} // namespace mc
