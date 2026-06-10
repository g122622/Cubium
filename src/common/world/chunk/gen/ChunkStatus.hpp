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
#include <set>
#include <string>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 区块类型
// ============================================================================

enum class ChunkType : u8 { PROTOCHUNK, LEVELCHUNK };

// ============================================================================
// 高度图类型标志
// ============================================================================

enum class HeightmapFlag : u32 {
    NONE = 0,
    WORLD_SURFACE_WG = 1 << 0,
    OCEAN_FLOOR_WG = 1 << 1,
    WORLD_SURFACE = 1 << 2,
    OCEAN_FLOOR = 1 << 3,
    MOTION_BLOCKING = 1 << 4,
    MOTION_BLOCKING_NO_LEAVES = 1 << 5,
    LIGHT_BLOCKING = 1 << 6,

    PRE_FEATURES = WORLD_SURFACE_WG | OCEAN_FLOOR_WG,
    POST_FEATURES = WORLD_SURFACE | OCEAN_FLOOR | MOTION_BLOCKING | MOTION_BLOCKING_NO_LEAVES
};

inline constexpr HeightmapFlag operator|(HeightmapFlag a, HeightmapFlag b)
{
    return static_cast<HeightmapFlag>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline constexpr HeightmapFlag operator&(HeightmapFlag a, HeightmapFlag b)
{
    return static_cast<HeightmapFlag>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline constexpr HeightmapFlag& operator|=(HeightmapFlag& a, HeightmapFlag b)
{
    a = a | b;
    return a;
}

inline constexpr bool hasFlag(HeightmapFlag flags, HeightmapFlag flag)
{
    return (static_cast<u32>(flags) & static_cast<u32>(flag)) != 0;
}

// ============================================================================
// 区块生成阶段
// ============================================================================

class ChunkStatus {
public:
    ChunkStatus() = default;

    ChunkStatus(
        const std::string& name, i32 ordinal, const ChunkStatus* parent, HeightmapFlag heightmaps, ChunkType type);

    [[nodiscard]] const std::string& name() const { return m_name; }
    [[nodiscard]] i32 ordinal() const { return m_ordinal; }
    [[nodiscard]] const ChunkStatus* parent() const { return m_parent; }
    [[nodiscard]] HeightmapFlag heightmaps() const { return m_heightmaps; }
    [[nodiscard]] ChunkType type() const { return m_type; }
    [[nodiscard]] HeightmapFlag heightmapsAfter() const { return m_heightmaps; }

    [[nodiscard]] bool isAtLeast(const ChunkStatus& status) const { return m_ordinal >= status.m_ordinal; }
    [[nodiscard]] bool isAfter(const ChunkStatus& status) const { return m_ordinal > status.m_ordinal; }
    [[nodiscard]] bool isOrBefore(const ChunkStatus& status) const { return m_ordinal <= status.m_ordinal; }
    [[nodiscard]] bool isBefore(const ChunkStatus& status) const { return m_ordinal < status.m_ordinal; }

    [[nodiscard]] static const ChunkStatus& max(const ChunkStatus& a, const ChunkStatus& b)
    {
        return a.isAfter(b) ? a : b;
    }

    [[nodiscard]] static const std::vector<ChunkStatus>& getAll();
    [[nodiscard]] static const ChunkStatus* byName(const std::string& name);
    [[nodiscard]] static const ChunkStatus* byOrdinal(i32 ordinal);
    [[nodiscard]] static i32 count() { return static_cast<i32>(getAll().size()); }

    bool operator==(const ChunkStatus& other) const { return m_ordinal == other.m_ordinal; }
    bool operator!=(const ChunkStatus& other) const { return m_ordinal != other.m_ordinal; }
    bool operator<(const ChunkStatus& other) const { return m_ordinal < other.m_ordinal; }
    bool operator<=(const ChunkStatus& other) const { return m_ordinal <= other.m_ordinal; }
    bool operator>(const ChunkStatus& other) const { return m_ordinal > other.m_ordinal; }
    bool operator>=(const ChunkStatus& other) const { return m_ordinal >= other.m_ordinal; }

private:
    std::string m_name;
    i32 m_ordinal = 0;
    const ChunkStatus* m_parent = nullptr;
    HeightmapFlag m_heightmaps = HeightmapFlag::NONE;
    ChunkType m_type = ChunkType::PROTOCHUNK;
};

namespace ChunkStatuses {

constexpr i32 EMPTY_ORDINAL = 0;
constexpr i32 STRUCTURE_STARTS_ORDINAL = 1;
constexpr i32 STRUCTURE_REFERENCES_ORDINAL = 2;
constexpr i32 BIOMES_ORDINAL = 3;
constexpr i32 NOISE_ORDINAL = 4;
constexpr i32 SURFACE_ORDINAL = 5;
constexpr i32 CARVERS_ORDINAL = 6;
constexpr i32 FEATURES_ORDINAL = 7;
constexpr i32 INITIALIZE_LIGHT_ORDINAL = 8;
constexpr i32 LIGHT_ORDINAL = 9;
constexpr i32 SPAWN_ORDINAL = 10;
constexpr i32 FULL_ORDINAL = 11;

constexpr i32 COUNT = 12;

extern const ChunkStatus EMPTY;
extern const ChunkStatus STRUCTURE_STARTS;
extern const ChunkStatus STRUCTURE_REFERENCES;
extern const ChunkStatus BIOMES;
extern const ChunkStatus NOISE;
extern const ChunkStatus SURFACE;
extern const ChunkStatus CARVERS;
extern const ChunkStatus FEATURES;
extern const ChunkStatus INITIALIZE_LIGHT;
extern const ChunkStatus LIGHT;
extern const ChunkStatus SPAWN;
extern const ChunkStatus FULL;

} // namespace ChunkStatuses

} // namespace mc::world::chunk

// 向后兼容的命名空间别名
namespace mc {
using ChunkType = mc::world::chunk::ChunkType;
using HeightmapFlag = mc::world::chunk::HeightmapFlag;
using ChunkStatus = mc::world::chunk::ChunkStatus;
namespace ChunkStatuses = mc::world::chunk::ChunkStatuses;
} // namespace mc
