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

// 在macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/fluid/Fluid.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::world::chunk {

// ============================================================================
// ChunkSection 实现
// ============================================================================

ChunkSection::ChunkSection()
    : m_memTrack(this)
    , m_blockStates() // PalettedContainer 默认 SingleValue(0=空气)
{}

ChunkSection::ChunkSection(ChunkSection&& other) noexcept
    : m_memTrack() // 默认构造为非活跃，body 中重绑定
    , m_blockStates(std::move(other.m_blockStates))
    , m_blockCount(other.m_blockCount)
    , m_needsRecalculate(other.m_needsRecalculate)
    , m_blockTickRefCount(other.m_blockTickRefCount)
    , m_fluidRefCount(other.m_fluidRefCount)
{
    // 对象级追踪重绑定：释放源地址、分配目标地址（守卫不可移动，故在 body 处理，
    // 初始化列表中默认构造为非活跃）。若不重绑定，move 后源地址仍留在 Tracy 活跃集，
    // 堆复用该地址时触发 MemAllocTwice 硬失败。
    other.m_memTrack.unbind();
    m_memTrack.bind(this);
}

ChunkSection& ChunkSection::operator=(ChunkSection&& other) noexcept
{
    if (this != &other) {
        m_blockStates = std::move(other.m_blockStates);
        m_blockCount = other.m_blockCount;
        m_needsRecalculate = other.m_needsRecalculate;
        m_blockTickRefCount = other.m_blockTickRefCount;
        m_fluidRefCount = other.m_fluidRefCount;

        // 对象级追踪重绑定（同 move ctor 语义）：释放双方旧地址、目标重新绑定新地址
        m_memTrack.unbind();
        other.m_memTrack.unbind();
        m_memTrack.bind(this);
    }
    return *this;
}

void ChunkSection::_updateCounters(u32 oldStateId, u32 newStateId)
{
    const BlockState* oldState = Block::getBlockState(oldStateId);
    const BlockState* newState = Block::getBlockState(newStateId);

    bool oldIsAir = oldState ? oldState->isAir() : true;
    bool newIsAir = newState ? newState->isAir() : true;

    if (oldIsAir && !newIsAir) {
        ++m_blockCount;
    } else if (!oldIsAir && newIsAir) {
        --m_blockCount;
    }

    if (oldState && oldState->getBlock().ticksRandomly()) {
        --m_blockTickRefCount;
    }
    if (newState && newState->getBlock().ticksRandomly()) {
        ++m_blockTickRefCount;
    }

    if (oldState) {
        const fluid::FluidState* oldFluid = oldState->getFluidState();
        if (oldFluid && !oldFluid->isEmpty()) {
            --m_fluidRefCount;
        }
    }
    if (newState) {
        const fluid::FluidState* newFluid = newState->getFluidState();
        if (newFluid && !newFluid->isEmpty()) {
            ++m_fluidRefCount;
        }
    }
}

u32 ChunkSection::getBlockStateId(i32 x, i32 y, i32 z) const
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return 0; // 空气
    }
    return m_blockStates.get(blockIndex(x, y, z));
}

void ChunkSection::setBlockStateIdFast(i32 index, u32 stateId)
{
    if (index < 0 || index >= VOLUME) {
        return;
    }

    u32 oldStateId = m_blockStates.getAndSet(index, stateId);
    _updateCounters(oldStateId, stateId);
}

void ChunkSection::setBlockStateId(i32 x, i32 y, i32 z, u32 stateId)
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return;
    }
    i32 index = blockIndex(x, y, z);
    u32 oldStateId = m_blockStates.getAndSet(index, stateId);
    _updateCounters(oldStateId, stateId);
    m_needsRecalculate = true;
}

const BlockState* ChunkSection::getBlockState(i32 x, i32 y, i32 z) const
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return nullptr;
    }

    u32 stateId = getBlockStateId(x, y, z);
    return Block::getBlockState(stateId);
}

void ChunkSection::rebuildTickCounters()
{
    m_blockTickRefCount = 0;
    m_fluidRefCount = 0;

    m_blockStates.forEach([this](i32 /*index*/, u32 stateId) {
        const BlockState* state = Block::getBlockState(stateId);
        if (state == nullptr) {
            return;
        }
        if (!state->isAir() && state->getBlock().ticksRandomly()) {
            ++m_blockTickRefCount;
        }
        const fluid::FluidState* fluidState = state->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            ++m_fluidRefCount;
        }
    });
}

void ChunkSection::setBlockState(i32 x, i32 y, i32 z, const BlockState* state)
{
    u32 stateId = state ? state->stateId() : 0;
    setBlockStateId(x, y, z, stateId);
}

std::vector<u8> ChunkSection::serialize() const
{
    // 格式: 块数量 + 方块状态ID
    // 光照不在本类中（归 ChunkData 的 SWMRNibbleArray），故不落在这里。
    constexpr size_t SECTION_DATA_SIZE = 2 + VOLUME * sizeof(u32);

    std::vector<u8> data(SECTION_DATA_SIZE);
    u8* out = data.data();

    // 块数量
    *out++ = static_cast<u8>(m_blockCount >> 8);
    *out++ = static_cast<u8>(m_blockCount & 0xFF);

    // 方块状态ID (u32) — 通过 toFlat() 从调色板导出为扁平 u32 数组
    auto flat = m_blockStates.toFlat();
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    std::memcpy(out, flat.data(), VOLUME * sizeof(u32));
#else
    for (u32 stateId : flat) {
        *out++ = static_cast<u8>(stateId & 0xFF);
        *out++ = static_cast<u8>((stateId >> 8) & 0xFF);
        *out++ = static_cast<u8>((stateId >> 16) & 0xFF);
        *out++ = static_cast<u8>((stateId >> 24) & 0xFF);
    }
#endif

    return data;
}

Result<std::unique_ptr<ChunkSection>> ChunkSection::deserialize(const u8* data, size_t size)
{
    // 新格式大小: 2 + VOLUME * 4
    constexpr size_t expectedSize = 2 + VOLUME * sizeof(u32);
    if (size < expectedSize) [[unlikely]] {
        std::stringstream ss;
        ss << "Invalid section data size, expected at least " << expectedSize << " bytes, got " << size << " bytes";
        return Error(ErrorCode::InvalidArgument, ss.str());
    }

    auto section = std::make_unique<ChunkSection>();
    size_t offset = 0;

    // 块数量
    const u16 storedBlockCount = (static_cast<u16>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    // 方块状态ID — 从扁平 u32 数组加载到调色板容器
    std::vector<u32> blockStates(VOLUME);
    const size_t blockStateBytes = VOLUME * sizeof(u32);
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    std::memcpy(blockStates.data(), data + offset, blockStateBytes);
#else
    for (size_t i = 0; i < VOLUME; ++i) {
        blockStates[i] = static_cast<u32>(data[offset]) | (static_cast<u32>(data[offset + 1]) << 8) |
            (static_cast<u32>(data[offset + 2]) << 16) | (static_cast<u32>(data[offset + 3]) << 24);
        offset += 4;
    }
#endif
    section->m_blockStates.fromFlat(blockStates.data(), VOLUME);
    offset += blockStateBytes;

    section->rebuildTickCounters();

    // 非空方块计数不直接采信外部值，而是从刚载入的方块数据实测得出。
    //
    // 该计数是 isEmpty() 的唯一判据，而 isEmpty() 决定序列化是否整段跳过方块数据：若外部
    // 数据给出"计数为 0"而调色板里实际有方块，本段会在下次保存时被静默清空、读回全是空气。
    // 实测重算的代价是 O(VOLUME) 一次线性扫描，而它换掉的是"整段地形无声消失"这一类故障。
    u16 measuredBlockCount = 0;
    section->m_blockStates.forEach([&measuredBlockCount](i32 /*index*/, u32 stateId) {
        const BlockState* state = Block::getBlockState(stateId);
        if (state != nullptr && !state->isAir()) {
            ++measuredBlockCount;
        }
    });
    section->m_blockCount = measuredBlockCount;

    if (measuredBlockCount != storedBlockCount) {
        spdlog::warn("ChunkSection deserialize: stored block count {} disagrees with the actual {} "
                     "non-air blocks in the section data; using the measured value",
            storedBlockCount,
            measuredBlockCount);
    }

    return std::move(section);
}

void ChunkSection::fill(u32 stateId)
{
    m_blockStates.fill(stateId);

    const BlockState* state = Block::getBlockState(stateId);
    m_blockCount = (state && !state->isAir()) ? VOLUME : 0;
    rebuildTickCounters();
    m_needsRecalculate = true;
}

} // namespace mc::world::chunk
