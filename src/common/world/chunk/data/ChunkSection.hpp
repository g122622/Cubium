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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/MemoryTracking.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/data/PalettedContainer.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 区块段 (16x16x16 方块)
// ============================================================================

class ChunkSection {
public:
    static constexpr i32 SIZE = mc::world::CHUNK_SECTION_HEIGHT; // 16
    static constexpr i32 VOLUME = SIZE * SIZE * SIZE;            // 4096

    ChunkSection();
    ~ChunkSection() = default;

    // 不可拷贝（含对象追踪守卫）；显式移动（守卫不可移动，须在 body 重绑定）
    ChunkSection(const ChunkSection&) = delete;
    ChunkSection& operator=(const ChunkSection&) = delete;
    ChunkSection(ChunkSection&& other) noexcept;
    ChunkSection& operator=(ChunkSection&& other) noexcept;

    // 方块访问 (使用状态ID)
    [[nodiscard]] u32 getBlockStateId(i32 x, i32 y, i32 z) const;
    void setBlockStateId(i32 x, i32 y, i32 z, u32 stateId);

    // 方块访问 (使用 BlockState 指针)
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const;
    void setBlockState(i32 x, i32 y, i32 z, const BlockState* state);

    // 快速访问 (无边界检查)
    [[nodiscard]] u32 getBlockStateIdFast(i32 index) const
    {
        if (index < 0 || index >= VOLUME) {
            return 0;
        }
        return m_blockStates.get(index);
    }
    void setBlockStateIdFast(i32 index, u32 stateId);

    // 索引计算
    [[nodiscard]] static i32 blockIndex(i32 x, i32 y, i32 z) { return y * SIZE * SIZE + z * SIZE + x; }

    // 段信息
    [[nodiscard]] bool isEmpty() const { return m_blockCount == 0; }
    [[nodiscard]] u16 getBlockCount() const { return m_blockCount; }
    void setBlockCount(u16 count) { m_blockCount = count; }

    /**
     * @brief 获取方块状态调色板容器的只读引用
     *
     * 供 vanilla 1.21.11 level_chunk_with_light IR 构建层（VanillaChunkWire）读取段内
     * 方块状态的 palette/storage，按 vanilla wire 规则重新打包。容器内存的是项目内部
     * stateId，调用方负责经 JavaBlockStateIdMap 翻译为 Java 全局 block state id。
     */
    [[nodiscard]] const PalettedContainer& blockStates() const noexcept { return m_blockStates; }

    void rebuildTickCounters();
    [[nodiscard]] bool needsRecalculate() const { return m_needsRecalculate; }
    void setNeedsRecalculate(bool value) { m_needsRecalculate = value; }

    // 随机刻计数器（用于性能优化）
    [[nodiscard]] bool needsRandomTickAny() const { return m_blockTickRefCount > 0 || m_fluidRefCount > 0; }
    [[nodiscard]] bool needsRandomTick() const { return m_blockTickRefCount > 0; }
    [[nodiscard]] bool needsRandomTickFluid() const { return m_fluidRefCount > 0; }
    [[nodiscard]] u16 blockTickRefCount() const { return m_blockTickRefCount; }
    [[nodiscard]] u16 fluidRefCount() const { return m_fluidRefCount; }

    // 光照
    //
    // 【为何本类不持有光照数据】对齐原版 1.21.11：LevelChunkSection 只含 states + biomes，
    // 光照（skyLight/blockLight）归 chunk 层。本项目对应 ChunkData 的 SWMRNibbleArray
    // （skyNibbles()/blockNibbles()），段索引经 world::sectionIndexToCoord 换算到光照段坐标后
    // 取 LIGHT_SECTIONS 数组中的对应项。
    //
    // 历史上本类内联了两份 NibbleArray（各 2048 B/段），由 ServerWorld::_syncLightDataToChunk
    // 从 SWMR 复制进来、仅供 SectionCodec 落盘读取，服务端从不读回——单玩家 1225 区块实测
    // 约 11.4 MB 纯冗余。删除后光照只有 SWMR 一份权威副本。

    // 序列化
    [[nodiscard]] std::vector<u8> serialize() const;
    [[nodiscard]] static Result<std::unique_ptr<ChunkSection>> deserialize(const u8* data, size_t size);

    // 填充
    void fill(u32 stateId);

private:
    // 对象级内存追踪守卫：绑定本对象地址，ctor 发 alloc、dtor 发 free。move 时由
    // move ctor/assign 显式「释放旧地址 + 分配新地址」重绑定（守卫不可移动）。
    // 仅 MC_ENABLE_MEMORY && MC_ENABLE_TRACY 时发事件，其余分支空操作。
    ::mc::profiler::TracyObjectTracker<"ChunkSection"> m_memTrack;

    // 调色板压缩存储方块状态 ID（SingleValue/Linear/HashMap/Flat 自适应）
    // 替代原扁平 std::vector<u32> (16 KB/段)，典型段内存降至 2-4 KB
    PalettedContainer m_blockStates;
    u16 m_blockCount = 0; // 非空气方块数量
    bool m_needsRecalculate = false;

    // 随机刻计数器（用于性能优化）
    u16 m_blockTickRefCount = 0; // ticksRandomly 方块数量
    u16 m_fluidRefCount = 0;     // 流体方块数量

    // 更新方块计数器（blockCount、blockTickRefCount、fluidRefCount）
    void _updateCounters(u32 oldStateId, u32 newStateId);
};

} // namespace mc::world::chunk
