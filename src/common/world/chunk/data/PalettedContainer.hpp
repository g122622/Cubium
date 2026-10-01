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
#include "common/profiler/MemoryTracking.hpp"

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 内存追踪：调色板内部 vector 用 TracyTrackingAlloc 追踪（按池分组）
//   - storage / hashMap 用 "ChunkPaletteStorage"（u64 位存储与哈希槽）
//   - palette 用 "ChunkPalette"（u32 调色板值）
// 分配器 is_always_equal=true，无状态，不影响 move 语义（PalettedContainer 仅 move
// 内部 vector）。
// ============================================================================
template <typename T>
using PaletteStorageAlloc = ::mc::profiler::TracyTrackingAlloc<T, "ChunkPaletteStorage">;
template <typename T>
using PaletteAlloc = ::mc::profiler::TracyTrackingAlloc<T, "ChunkPalette">;

// ============================================================================
// 调色板容器 — 方块状态存储
//
// 只有一种工作模式：**调色板 + 位压缩存储 + 开放寻址反向哈希表**
//   - palette：paletteIndex → stateId（u32 数组）
//   - storage：每格 bits 位的压缩存储，存 paletteIndex
//   - hashMap：stateId → paletteIndex 的反向映射，开放寻址 + 线性探测
//
// 位宽 bits = max(MIN_BITS, ceil(log2(唯一值个数)))，即从 1 位起步，不设 4 位下限。
//
// 均匀态（唯一值个数 == 1）：bits = 0 且 **不分配 storage / hashMap**，整段按
// palette[0] 取值——空气段与"整段同一方块"因此零额外内存（fill() 也是 O(1) 释放）。
//
// 磁盘/网络格式与本内部表示无关：读写方（JavaChunkReader / ChunkSerializer /
// VanillaChunkWire）各自按 MC 规则重新计算位数与打包方式，本类的 bits 只用于内部
// 内存布局。
//
// 参考: net.minecraft.world.level.chunk.PalettedContainer
//       ca.spottedleaf.moonrise.mixin.fast_palette
// ============================================================================

class PalettedContainer {
public:
    /**
     * @brief 容器元素数量 (16³ = 4096)
     */
    static constexpr i32 VOLUME = 4096;

    /**
     * @brief 最小位宽（唯一值个数 ≥ 2 时）
     *
     * 不再沿用原版"线性调色板至少 4 位"的下限：位宽直接取 ceil(log2(唯一值个数))，
     * 从 1 位起步（唯一值个数 == 1 的均匀态为 0 位且无 storage）。
     */
    static constexpr i32 MIN_BITS = 1;

    // ========================================================================
    // 构造 / 赋值
    // ========================================================================

    /**
     * @brief 构造为空段（均匀态：stateId 0=空气，不分配 storage / 哈希表）
     */
    PalettedContainer();

    // ========================================================================
    // 元素访问
    // ========================================================================

    /**
     * @brief 获取指定索引的方块状态 ID
     *
     * @param index 线性索引 (0 ~ VOLUME-1)
     * @return 方块状态 ID
     */
    [[nodiscard]] u32 get(i32 index) const;

    /**
     * @brief 设置指定索引的方块状态 ID，返回旧值
     *
     * @param index 线性索引 (0 ~ VOLUME-1)
     * @param value 新的方块状态 ID
     * @return 之前的方块状态 ID
     */
    u32 getAndSet(i32 index, u32 value);

    /**
     * @brief 设置指定索引的方块状态 ID（丢弃旧值）
     *
     * @param index 线性索引 (0 ~ VOLUME-1)
     * @param value 新的方块状态 ID
     */
    void set(i32 index, u32 value);

    // ========================================================================
    // 批量操作
    // ========================================================================

    /**
     * @brief 用单一值填充整个容器
     *
     * 回到均匀态：palette = {value}、bits = 0，并释放 storage 与哈希表。O(1)。
     *
     * @param value 填充值
     */
    void fill(u32 value);

    /**
     * @brief 导出为扁平 u32 数组
     *
     * 用于序列化（写盘/发网络包，写方再按 MC 规则重新打包）与批量遍历。
     *
     * @return 包含 VOLUME 个 stateId 的 vector
     */
    [[nodiscard]] std::vector<u32> toFlat() const;

    /**
     * @brief 从扁平 u32 数组加载
     *
     * 统计唯一值数量后建立调色板、位置存储与反向哈希表（唯一值时回到均匀态）。
     *
     * @param data 指向 VOLUME 个 u32 的数据
     * @param count 元素数量（必须为 VOLUME）
     */
    void fromFlat(const u32* data, i32 count);

    // ========================================================================
    // 遍历
    // ========================================================================

    /**
     * @brief 遍历所有元素（调色板索引 → stateId）
     *
     * @param visitor 接收 (index, stateId) 的回调
     */
    void forEach(const std::function<void(i32, u32)>& visitor) const;

    /**
     * @brief 遍历所有唯一调色板值
     *
     * @param visitor 接收 (paletteIndex, stateId) 的回调
     */
    void forEachPaletteValue(const std::function<void(i32, u32)>& visitor) const;

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 获取调色板大小（唯一值数量，均匀态为 1）
     */
    [[nodiscard]] i32 paletteSize() const;

    /**
     * @brief 获取当前位宽（均匀态为 0）
     */
    [[nodiscard]] i32 bitsPerEntry() const;

    /**
     * @brief 估算内存占用（字节）
     */
    [[nodiscard]] size_t estimatedMemoryUsage() const;

private:
    // ========================================================================
    // 内部数据
    // ========================================================================
    struct Data {
        // 位存储（小端，LSB-first，条目可跨 u64 字）
        // 均匀态（paletteSize == 1）下为空，所有位置按 palette[0] 取值
        std::vector<u64, PaletteStorageAlloc<u64>> storage;

        // 位宽（每个条目占用的 bit 数；均匀态为 0）
        i32 bits = 0;

        // 调色板：paletteIndex → stateId（paletteSize 之后的元素无效）
        std::vector<u32, PaletteAlloc<u32>> palette;

        // 有效调色板条目数
        i32 paletteSize = 0;

        // 反向映射 stateId → paletteIndex 的开放寻址哈希表
        // 槽存储 (paletteIndex + 1)，0 表示空槽；均匀态下为空
        std::vector<u32, PaletteStorageAlloc<u32>> hashMap;
        i32 hashMapCapacity = 0; // 容量（2 的幂）
        i32 hashMapMask = 0;     // hashMapCapacity - 1
    };

    Data m_data;

    // ========================================================================
    // 内部方法
    // ========================================================================

    /**
     * @brief 从位存储中读取指定索引的 paletteIndex
     */
    [[nodiscard]] i32 _readBits(i32 index) const;

    /**
     * @brief 向位存储中写入指定索引的 paletteIndex，返回旧值
     */
    i32 _writeBits(i32 index, i32 value);

    /**
     * @brief 扩容位宽（重打包 storage；不改变 paletteIndex 语义）
     *
     * @param newBits 新的位宽（> 当前位宽）
     */
    void _onResize(i32 newBits);

    /**
     * @brief 查找或插入 stateId 到调色板，返回 paletteIndex
     *
     * 未命中时追加到调色板（必要时升位宽、建哈希表）。
     *
     * @param value 方块状态 ID
     * @return paletteIndex
     */
    i32 _idFor(u32 value);

    /**
     * @brief 哈希表：查找 stateId 对应的 paletteIndex（-1 表示未找到）
     */
    [[nodiscard]] i32 _hashMapLookup(u32 value) const;

    /**
     * @brief 哈希表：插入 stateId → paletteIndex 映射
     */
    void _hashMapInsert(u32 value, i32 paletteIndex);

    /**
     * @brief 哈希表：按当前 paletteSize 重建（首次建立或扩容）
     */
    void _hashMapRebuild();

    /**
     * @brief 计算存储 count 个调色板条目所需的位宽（count ≥ 2 时 ceil(log2(count))，下限 MIN_BITS）
     */
    [[nodiscard]] static i32 _calculateBitsForCount(i32 count);

    /**
     * @brief 计算 bits 位下需要的 u64 字数
     */
    [[nodiscard]] static i32 _storageWordCount(i32 bits);
};

} // namespace mc::world::chunk
