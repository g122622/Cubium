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

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockPos.hpp"

#include <optional>

namespace mc::server {
class ServerWorld;
}

namespace mc::server::structure {

/**
 * @brief 结构定位算法门面（无状态静态类）
 *
 * 承接原 ServerWorld::findNearestStructure / findNearestMapStructure 的网格搜索算法。
 * 算法围绕 StructureSet/StructurePlacement：依据放置规则（RandomSpread 网格搜索 /
 * ConcentricRings 预计算环形位置）枚举候选区块，经 StructureCheck 缓存过滤后取距中心
 * 最近的结构位置。MC Java 中属 ChunkGenerator 职责，与世界状态容器无关，故下沉至此。
 *
 * 无状态：所有数据经 ServerWorld public accessor（seed()/chunkManager()）取，不持引用。
 */
class StructureLocator {
public:
    /**
     * @brief 查找距 center 最近的结构（按结构 ID）
     *
     * @param world 服务端世界（取 seed()/chunkManager()→generator()→structureCheck()）
     * @param center 搜索中心（方块坐标）
     * @param structureId 目标结构资源 ID
     * @param maxDistance 最大搜索半径（方块）
     * @param skipExisting 是否跳过已生成区块的精确校验
     * @return 最近结构方块坐标；无候选返回 nullopt
     */
    [[nodiscard]] static std::optional<BlockPos> findNearestStructure(const ServerWorld& world,
        const BlockPos& center,
        const ResourceLocation& structureId,
        i32 maxDistance,
        bool skipExisting);

    /**
     * @brief 查找距 center 最近的结构（按结构标签 ID，遍历标签内所有结构取最近）
     *
     * @param world 服务端世界
     * @param center 搜索中心（方块坐标）
     * @param tagId 结构标签资源 ID（如 "village" "monument"）
     * @param maxDistance 最大搜索半径（方块）
     * @param skipExisting 是否跳过已生成区块的精确校验
     * @return 最近结构方块坐标；标签未知或无候选返回 nullopt
     */
    [[nodiscard]] static std::optional<BlockPos> findNearestMapStructure(const ServerWorld& world,
        const BlockPos& center,
        const ResourceLocation& tagId,
        i32 maxDistance,
        bool skipExisting);
};

} // namespace mc::server::structure
