/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted copies of the following:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "server/world/ChunkSnapshot.hpp"

#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"

namespace mc::server {

// ============================================================================
// fromPrimer
// ============================================================================

ChunkSnapshot ChunkSnapshot::fromPrimer(
    const mc::world::chunk::ChunkPrimer& primer, const mc::world::chunk::ChunkStatus& status)
{
    const mc::world::chunk::ChunkData* srcData = primer.getChunkData();

    // 深拷贝方块状态数据：创建新 ChunkData，逐段拷贝
    auto newData = std::make_shared<mc::world::chunk::ChunkData>(primer.x(), primer.z());

    if (srcData != nullptr) {
        for (i32 i = 0; i < mc::world::CHUNK_SECTIONS; ++i) {
            const mc::world::chunk::ChunkSection* srcSection = srcData->getSection(i);
            if (srcSection != nullptr) {
                // createSection 返回可变 ChunkSection*，通过拷贝赋值深拷贝整段
                mc::world::chunk::ChunkSection* dstSection = newData->createSection(i);
                if (dstSection != nullptr) {
                    *dstSection = *srcSection;
                }
            }
        }
    }

    // 深拷贝生物群系（primer 的 m_biomes → ChunkData）
    newData->setBiomes(primer.getBiomes());

    // 深拷贝高度图（primer 的 m_heightmaps → 快照独立持有）
    // ChunkPrimer::getHeightmap 对缺失类型返回 dummy（空高度图），这里只拷贝已存在的类型。
    // 遍历所有 7 种 HeightmapType，通过 primer 的 m_heightmaps 拷贝。
    std::unordered_map<mc::world::chunk::HeightmapType, mc::world::chunk::Heightmap> heightmaps;
    const mc::world::chunk::HeightmapType allTypes[] = {mc::world::chunk::HeightmapType::WorldSurface,
        mc::world::chunk::HeightmapType::OceanFloor,
        mc::world::chunk::HeightmapType::MotionBlocking,
        mc::world::chunk::HeightmapType::MotionBlockingNoLeaves,
        mc::world::chunk::HeightmapType::WorldSurfaceWG,
        mc::world::chunk::HeightmapType::OceanFloorWG,
        mc::world::chunk::HeightmapType::LightBlocking};

    for (mc::world::chunk::HeightmapType type : allTypes) {
        const mc::world::chunk::Heightmap& src = primer.getHeightmap(type);
        // Heightmap 拷贝构造（m_heights 是 std::array，值拷贝）
        heightmaps.emplace(type, src);
    }

    return ChunkSnapshot(
        primer.x(), primer.z(), &status, std::move(newData), std::move(heightmaps), primer.getBiomes());
}

// ============================================================================
// heightmap 查询
// ============================================================================

const mc::world::chunk::Heightmap* ChunkSnapshot::heightmap(mc::world::chunk::HeightmapType type) const
{
    auto it = m_heightmaps.find(type);
    if (it == m_heightmaps.end()) {
        return nullptr;
    }
    return &it->second;
}

} // namespace mc::server
