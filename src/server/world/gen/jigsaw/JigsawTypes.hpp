/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "JigsawOrientation.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief Jigsaw 放置行为（投影类型）
 *
 * 对应 MC 1.21 的 StructureTemplatePool.Projection。
 * - TerrainMatching：匹配地形高度，放置时应用 GravityStructureProcessor
 * - Rigid：固定位置，不应用重力处理器
 */
enum class JigsawPlacementBehaviour : u8 {
    TerrainMatching, ///< 匹配地形高度
    Rigid            ///< 固定位置
};

/**
 * @brief Jigsaw 连接类型
 *
 * - Rollable：可旋转连接，只需 facing 相反
 * - Aligned：对齐连接，facing 相反且 rotation 必须相同
 */
enum class JigsawJointType : u8 {
    Rollable, ///< 可旋转连接
    Aligned   ///< 对齐连接
};

/**
 * @brief Jigsaw 连接点信息
 *
 * 描述一个拼图块上的 Jigsaw 方块连接点，包含源位置、名称、目标池、目标名称、
 * 投影类型、连接类型、朝向以及优先级信息。
 *
 * selectionPriority 控制同一拼图块内连接点的处理顺序（降序，高优先级先处理）。
 * placementPriority 控制该连接点生成的子拼图块在组装队列中的出队顺序（降序，高优先级先出队）。
 */
struct JigsawJoint {
    BlockPos sourcePos;     ///< 源位置（在拼图块内）
    std::string sourceName; ///< 源连接点名称（nbt 中的 "name" 字段）
    std::string targetPool; ///< 目标模板池名称（nbt 中的 "pool" 字段）
    std::string targetName; ///< 目标连接点名称（nbt 中的 "target" 字段，"minecraft:empty" 表示终止）
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    JigsawJointType jointType = JigsawJointType::Rollable;      ///< 连接类型
    JigsawOrientation orientation = JigsawOrientation::NorthUp; ///< Jigsaw 方块朝向
    i32 sourceGroundY = 0;                                      ///< 源地面高度
    i32 selectionPriority = 0;                                  ///< 选择优先级（同一拼图块内，降序）
    i32 placementPriority = 0;                                  ///< 放置优先级（组装队列中，降序）

    JigsawJoint() = default;
    JigsawJoint(const BlockPos& src,
        const std::string& srcName,
        const std::string& pool,
        const std::string& tgtName,
        JigsawPlacementBehaviour proj = JigsawPlacementBehaviour::Rigid)
        : sourcePos(src)
        , sourceName(srcName)
        , targetPool(pool)
        , targetName(tgtName)
        , projection(proj)
    {}
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
