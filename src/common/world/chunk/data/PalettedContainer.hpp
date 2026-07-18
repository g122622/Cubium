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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
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

#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 内存追踪：调色板内部 vector 用 TracyTrackingAlloc 追踪（按池分组）
//   - storage / hashMap 用 "ChunkPaletteStorage"（u64/u32 位存储与哈希槽）
//   - palette 用 "ChunkPalette"（u32 调色板值）
// 分配器 is_always_equal=true，无状态，不影响 move 语义（PalettedContainer 仅 move
// unique_ptr<Data>，不触碰内部 vector）。仅 MC_ENABLE_MEMORY && MC_ENABLE_TRACY 时发事件。
// ============================================================================
template <typename T>
using PaletteStorageAlloc = ::mc::profiler::TracyTrackingAlloc<T, "ChunkPaletteStorage">;
template <typename T>
using PaletteAlloc = ::mc::profiler::TracyTrackingAlloc<T, "ChunkPalette">;

// ============================================================================
// 调色板容器 — 方块状态存储的内存优化替代方案
//
// 替代 ChunkSection 中扁平的 std::vector<u32> (16 KB/段)，
// 使用调色板 + 位压缩存储，典型情况下将内存占用降低 5-10 倍。
//
// 调色板模式：
//   SingleValue — 0-1 种唯一值，无 storage 数组（空气段趋近 0 字节）
//   Linear      — 2-15 种唯一值，线性扫描调色板，bitsPerEntry = max(4, ceil(log2(count)))
//   HashMap     — 16+ 种唯一值，哈希双射调色板，bitsPerEntry = ceil(log2(count))
//   Flat        — bitsPerEntry >= MIN_BITS_FOR_FLAT 时退化直存，与原 vector<u32> 等价
//
// 参考: net.minecraft.world.level.chunk.PalettedContainer (MC 1.16.5)
//       ca.spottedleaf.moonrise.mixin.fast_palette (Moonrise fast_palette 优化)
// ============================================================================

class PalettedContainer {
public:
    /**
     * @brief 容器元素数量 (16³ = 4096)
     */
    static constexpr i32 VOLUME = 4096;

    /**
     * @brief 调色板最小位数（对齐原版 MC，线性调色板至少 4 位）
     */
    static constexpr i32 MIN_BITS = 4;

    /**
     * @brief 退化直存的位数阈值（bitsPerEntry >= 此值时直接存 stateId）
     */
    static constexpr i32 MIN_BITS_FOR_FLAT = 16;

    // ========================================================================
    // 构造 / 赋值
    // ========================================================================

    PalettedContainer();

    ~PalettedContainer() = default;

    PalettedContainer(const PalettedContainer& other);
    PalettedContainer(PalettedContainer&& other) noexcept;
    PalettedContainer& operator=(const PalettedContainer& other);
    PalettedContainer& operator=(PalettedContainer&& other) noexcept;

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
     * 重置为 SingleValue 模式，不分配 storage。O(1)。
     *
     * @param value 填充值
     */
    void fill(u32 value);

    /**
     * @brief 导出为扁平 u32 数组
     *
     * 用于序列化（与磁盘格式兼容）和批量遍历。
     *
     * @return 包含 VOLUME 个 stateId 的 vector
     */
    [[nodiscard]] std::vector<u32> toFlat() const;

    /**
     * @brief 从扁平 u32 数组加载
     *
     * 分析数据中的唯一值数量，选择最优调色板模式。
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
     * @brief 获取调色板大小（唯一值数量）
     */
    [[nodiscard]] i32 paletteSize() const;

    /**
     * @brief 获取当前位数
     */
    [[nodiscard]] i32 bitsPerEntry() const;

    /**
     * @brief 获取调色板中指定索引的值
     *
     * @param paletteIndex 调色板索引
     * @return 方块状态 ID
     */
    [[nodiscard]] u32 paletteValue(i32 paletteIndex) const;

    /**
     * @brief 获取原始调色板数组指针（fast_palette 优化）
     *
     * 调色板变更后指针可能失效，必须重新获取。
     *
     * @return 指向调色板数组的指针，SingleValue 模式下返回内部单元素数组
     */
    [[nodiscard]] const u32* rawPalette() const;

    /**
     * @brief 获取位存储数据（只读，用于高级序列化）
     */
    [[nodiscard]] const std::vector<u64, PaletteStorageAlloc<u64>>& storage() const;

    /**
     * @brief 估算内存占用（字节）
     */
    [[nodiscard]] size_t estimatedMemoryUsage() const;

private:
    // ========================================================================
    // 调色板模式
    // ========================================================================
    enum class Mode : u8 {
        SingleValue, ///< 0-1 种唯一值，无 storage
        Linear,      ///< 2-15 种唯一值，线性扫描
        HashMap,     ///< 16+ 种唯一值，哈希双射
        Flat         ///< bitsPerEntry >= MIN_BITS_FOR_FLAT，直存
    };

    // ========================================================================
    // 内部数据
    // ========================================================================
    struct Data {
        Mode mode = Mode::SingleValue;

        // 位存储（小端，LSB-first）
        // SingleValue 模式下为空；其他模式存储调色板索引
        std::vector<u64, PaletteStorageAlloc<u64>> storage;

        // 位数（每个条目占用的 bit 数）
        i32 bits = 0;

        // 调色板值数组
        // SingleValue: 仅 m_palette[0] 有效
        // Linear: 按插入顺序
        // HashMap: 稀疏数组（index → value），配 m_paletteSize 使用
        // Flat: 空（stateId 直接存于 storage）
        std::vector<u32, PaletteAlloc<u32>> palette;

        // 实际调色板条目数（HashMap 模式下可能 < palette.size()）
        i32 paletteSize = 0;

        // SingleValue 专用：单一值（mode == SingleValue 时使用）
        u32 singleValue = 0;

        // fast_palette 缓存：指向 palette.data() 的原始指针
        // 每次 resize/mutation 后刷新
        const u32* rawPalettePtr = nullptr;

        // HashMap 专用：反向映射 stateId → paletteIndex
        // 使用开放寻址哈希表，避免 std::unordered_map 开销
        std::vector<u32, PaletteStorageAlloc<u32>> hashMap; // 哈希槽，存储 (paletteIndex + 1)，0 表示空
        i32 hashMapCapacity = 0;                            // 哈希表容量（2 的幂）
        i32 hashMapMask = 0;                                // hashMapCapacity - 1

        Data() = default;

        /**
         * @brief 刷新 fast_palette 缓存指针
         */
        void refreshRawPalette();
    };

    std::unique_ptr<Data> m_data;

    // ========================================================================
    // 内部方法
    // ========================================================================

    /**
     * @brief 获取调色板索引对应的 stateId
     */
    [[nodiscard]] u32 _paletteLookup(i32 paletteIndex) const;

    /**
     * @brief 查找或插入 stateId 到调色板，返回调色板索引
     *
     * 如果调色板已满（需要更多位数），触发 resize。
     *
     * @param value 方块状态 ID
     * @return 调色板索引
     */
    i32 _idFor(u32 value);

    /**
     * @brief 从位存储中读取指定索引的值
     */
    [[nodiscard]] i32 _readBits(i32 index) const;

    /**
     * @brief 向位存储中写入指定索引的值，返回旧值
     */
    i32 _writeBits(i32 index, i32 value);

    /**
     * @brief 从位存储中读取并写入指定索引的值，返回旧值
     */
    i32 _getAndSetBits(i32 index, i32 value);

    /**
     * @brief 扩容调色板（升级位数和模式）
     *
     * @param newBits 新的位数
     */
    void _onResize(i32 newBits);

    /**
     * @brief 从 SingleValue 转换为 Linear
     */
    void _transitionSingleToLinear(u32 existingValue, u32 newValue, i32 index);

    /**
     * @brief HashMap: 查找 stateId 对应的调色板索引（-1 表示未找到）
     */
    [[nodiscard]] i32 _hashMapLookup(u32 value) const;

    /**
     * @brief HashMap: 插入 stateId → paletteIndex 映射
     */
    void _hashMapInsert(u32 value, i32 paletteIndex);

    /**
     * @brief HashMap: 重建哈希表（扩容或全量重建）
     */
    void _hashMapRebuild();

    /**
     * @brief 计算存储给定值所需的位数
     */
    [[nodiscard]] static i32 _calculateBitsForValue(u32 value);

    /**
     * @brief 计算存储 count 个调色板条目所需的位数
     */
    [[nodiscard]] static i32 _calculateBitsForCount(i32 count);

    /**
     * @brief 计算 bits 位下需要的 u64 字数
     */
    [[nodiscard]] static i32 _storageWordCount(i32 bits);

    /**
     * @brief 根据当前调色板大小决定调色板模式
     */
    [[nodiscard]] static Mode _modeForBits(i32 bits);
};

} // namespace mc::world::chunk
