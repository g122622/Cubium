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

#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 静态成员初始化
// ============================================================================

namespace ChunkStatuses {

const ChunkStatus EMPTY("empty", EMPTY_ORDINAL, nullptr, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

const ChunkStatus STRUCTURE_STARTS(
    "structure_starts", STRUCTURE_STARTS_ORDINAL, &EMPTY, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

const ChunkStatus STRUCTURE_REFERENCES("structure_references",
    STRUCTURE_REFERENCES_ORDINAL,
    &STRUCTURE_STARTS,
    HeightmapFlag::PRE_FEATURES,
    ChunkType::PROTOCHUNK);

const ChunkStatus BIOMES(
    "biomes", BIOMES_ORDINAL, &STRUCTURE_REFERENCES, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

const ChunkStatus NOISE("noise", NOISE_ORDINAL, &BIOMES, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

const ChunkStatus SURFACE("surface", SURFACE_ORDINAL, &NOISE, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

const ChunkStatus CARVERS("carvers", CARVERS_ORDINAL, &SURFACE, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

const ChunkStatus FEATURES("features", FEATURES_ORDINAL, &CARVERS, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

const ChunkStatus INITIALIZE_LIGHT(
    "initialize_light", INITIALIZE_LIGHT_ORDINAL, &FEATURES, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

const ChunkStatus LIGHT("light", LIGHT_ORDINAL, &INITIALIZE_LIGHT, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

const ChunkStatus SPAWN("spawn", SPAWN_ORDINAL, &LIGHT, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

const ChunkStatus FULL("full", FULL_ORDINAL, &SPAWN, HeightmapFlag::POST_FEATURES, ChunkType::LEVELCHUNK);

} // namespace ChunkStatuses

// ============================================================================
// 构造函数
// ============================================================================

ChunkStatus::ChunkStatus(
    const std::string& name, i32 ordinal, const ChunkStatus* parent, HeightmapFlag heightmaps, ChunkType type)
    : m_name(name)
    , m_ordinal(ordinal)
    , m_parent(parent ? parent : this)
    , m_heightmaps(heightmaps)
    , m_type(type)
{}

// ============================================================================
// 静态方法
// ============================================================================

const std::vector<ChunkStatus>& ChunkStatus::getAll()
{
    // TODO: 这里的元素是 ChunkStatuses::X 的**副本**（聚合拷贝到堆上的 vector），因此调用方能取到
    // 指向副本的指针而非各状态单例的指针。目前所有比较都走 ordinal()，功能正确；但持有 ChunkStatus
    // 指针的调用方（SingleChunkLifecycleManager::m_currentGenStatus/m_scheduledStatus、
    // PendingReschedule::target 等）隐含"指针即静态单例"的假设，一旦引入指针相等比较就会静默失效。
    // 应改为保存指向单例的指针（如 std::vector<const ChunkStatus*>）。
    static const std::vector<ChunkStatus> allStatuses = {ChunkStatuses::EMPTY,
        ChunkStatuses::STRUCTURE_STARTS,
        ChunkStatuses::STRUCTURE_REFERENCES,
        ChunkStatuses::BIOMES,
        ChunkStatuses::NOISE,
        ChunkStatuses::SURFACE,
        ChunkStatuses::CARVERS,
        ChunkStatuses::FEATURES,
        ChunkStatuses::INITIALIZE_LIGHT,
        ChunkStatuses::LIGHT,
        ChunkStatuses::SPAWN,
        ChunkStatuses::FULL};
    return allStatuses;
}

const ChunkStatus* ChunkStatus::byName(const std::string& name)
{
    const auto& all = getAll();
    for (const auto& status : all) {
        if (status.name() == name) {
            return &status;
        }
    }
    return nullptr;
}

const ChunkStatus* ChunkStatus::byOrdinal(i32 ordinal)
{
    const auto& all = getAll();
    if (ordinal >= 0 && ordinal < static_cast<i32>(all.size())) {
        return &all[static_cast<size_t>(ordinal)];
    }
    return nullptr;
}

} // namespace mc::world::chunk
