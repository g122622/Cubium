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
#include <cstddef>
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

namespace detail {

/**
 * @brief 求 ceil(log2(n))，n <= 1 时返回 0
 *
 * 与 MC 的 Mth.ceillog2 同语义，用于在编译期推导高度图位存储的位宽。
 */
[[nodiscard]] constexpr i32 ceilLog2(i32 n) noexcept
{
    if (n <= 1) {
        return 0;
    }
    i32 bits = 0;
    for (i32 value = n - 1; value > 0; value >>= 1) {
        ++bits;
    }
    return bits;
}

} // namespace detail

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
 *
 * 【位压缩存储】底层用 9 bit/列的紧凑位存储（256 列共 37 个 u64 = 296 字节），
 * 而非扁平的 256 × i32（1028 字节）。位宽与布局对齐原版 1.21.11 的
 * `SimpleBitStorage`：每 64 bit 字存 64/BITS 个条目、条目不跨字边界。
 * 每列存的是 `raw - NO_BLOCK_SENTINEL`，故哨兵编码为 0、MIN_BUILD_HEIGHT 为 1，
 * 既保留了"无方块"与"minY 处有方块"的区分，又让默认全 0 的位存储天然等价于
 * "全部无方块"。7 张高度图合计从 7196 B/区块降至 2072 B/区块。
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

    // ========================================================================
    // 位存储布局（编译期由高度限制导出，改 MIN/MAX_BUILD_HEIGHT 无需手工同步）
    // ========================================================================

    /// 每列的位宽：需覆盖 encoded ∈ [0, MAX_BUILD_HEIGHT - NO_BLOCK_SENTINEL]（主世界 385）
    static constexpr i32 BITS = detail::ceilLog2(mc::world::MAX_BUILD_HEIGHT - NO_BLOCK_SENTINEL + 1);

    /// 每个 u64 字容纳的条目数（条目不跨字，故向下取整，字尾余位弃用）
    static constexpr i32 VALUES_PER_LONG = 64 / BITS;

    /// 位存储占用的 u64 字数
    static constexpr i32 WORD_COUNT = (SIZE + VALUES_PER_LONG - 1) / VALUES_PER_LONG;

    /// 单列编码的位掩码
    static constexpr u64 VALUE_MASK = (1ULL << BITS) - 1;

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
     *
     * 超出 [NO_BLOCK_SENTINEL, MAX_BUILD_HEIGHT] 的值会被夹到边界：位存储无法表示
     * 越界值，而调用方可能传入来自存档/网络的不可信数据（见 setData）。
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
    void setAll(BlockCoord value);

    /**
     * @brief 物化高度数据
     *
     * 位存储无法按引用交出 256 列的 i32 视图，故按值返回。单次调用在栈上产生
     * 1028 字节临时数组，调用频率为"每区块每类型若干次"（落盘、网络同步、FULL 收尾），
     * 不进入稳态驻留集。
     */
    [[nodiscard]] std::array<BlockCoord, SIZE> getData() const;

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
    std::array<u64, WORD_COUNT> m_words{};

    /// 把内部存储值（Y+1 或哨兵）编码为位存储值。越界值夹到边界。
    [[nodiscard]] static i32 _encode(BlockCoord raw) noexcept
    {
        if (raw < NO_BLOCK_SENTINEL) {
            return 0;
        }
        // 夹到"最高方块"对应的编码值，而非位存储的满值：9 bit 能表示到 511，但
        // [MAX_BUILD_HEIGHT+1, 511] 段并非合法高度。夹到满值会让越界输入变成
        // 一个位存储能存、语义却非法的值，后续读出来继续污染地形。
        constexpr i32 MAX_ENCODED = mc::world::MAX_BUILD_HEIGHT - NO_BLOCK_SENTINEL;
        static_assert(static_cast<u64>(MAX_ENCODED) <= VALUE_MASK, "位宽不足以表示最高方块");
        const i32 encoded = raw - NO_BLOCK_SENTINEL;
        return encoded > MAX_ENCODED ? MAX_ENCODED : encoded;
    }

    /// 把位存储值解码回内部存储值（Y+1 或哨兵）
    [[nodiscard]] static BlockCoord _decode(i32 encoded) noexcept
    {
        return static_cast<BlockCoord>(encoded) + NO_BLOCK_SENTINEL;
    }

    /// 读取指定列的编码值（无边界检查，调用方保证 index ∈ [0, SIZE)）
    [[nodiscard]] i32 _getEncoded(i32 index) const noexcept
    {
        const i32 cell = index / VALUES_PER_LONG;
        const i32 offset = (index - cell * VALUES_PER_LONG) * BITS;
        return static_cast<i32>((m_words[static_cast<size_t>(cell)] >> offset) & VALUE_MASK);
    }

    /// 写入指定列的编码值（无边界检查，同 _getEncoded）
    void _setEncoded(i32 index, i32 encoded) noexcept
    {
        const i32 cell = index / VALUES_PER_LONG;
        const i32 offset = (index - cell * VALUES_PER_LONG) * BITS;
        auto& word = m_words[static_cast<size_t>(cell)];
        word = (word & ~(VALUE_MASK << offset)) | ((static_cast<u64>(encoded) & VALUE_MASK) << offset);
    }

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
