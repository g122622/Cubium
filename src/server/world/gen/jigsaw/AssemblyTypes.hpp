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

#include "JigsawJunction.hpp"
#include "JigsawPiece.hpp"
#include "JigsawTypes.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {

class VoxelShape;

namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 已放置的拼图块信息
 *
 * 组装算法的中间产物，记录一个拼图块在世界中的放置位置、旋转、边界框和连接点。
 * 放置阶段（FEATURES）由 JigsawPlacer 遍历 PlacedPiece 调用 piece->place() 写入方块。
 */
struct PlacedPiece {
    std::unique_ptr<JigsawPiece> piece;
    BlockPos position;
    Rotation rotation = Rotation::None;
    Mirror mirror = Mirror::None;
    i32 groundLevelDelta = 0;
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    structure::StructureBoundingBox boundingBox;
    std::vector<JigsawJoint> joints;       ///< 已变换的连接点
    std::vector<JigsawJunction> junctions; ///< JigsawJunction 列表（用于 NoiseChunkGenerator 地形适配）

    PlacedPiece() = default;
    PlacedPiece(std::unique_ptr<JigsawPiece> p,
        const BlockPos& pos,
        Rotation rot,
        Mirror mir,
        i32 delta,
        const structure::StructureBoundingBox& box)
        : piece(std::move(p))
        , position(pos)
        , rotation(rot)
        , mirror(mir)
        , groundLevelDelta(delta)
        , boundingBox(box)
    {}

    // 移动构造和移动赋值
    PlacedPiece(PlacedPiece&& other) noexcept = default;
    PlacedPiece& operator=(PlacedPiece&& other) noexcept = default;

    // 禁用拷贝
    PlacedPiece(const PlacedPiece&) = delete;
    PlacedPiece& operator=(const PlacedPiece&) = delete;
};

/**
 * @brief 待处理的连接点
 *
 * 组装队列中的元素，记录一个待匹配的连接点信息。
 * placementPriority 控制出队顺序（降序，高优先级先出队），对应 MC 1.21 的 SequencedPriorityIterator。
 *
 * parentMinY / parentGroundLevelDelta 记录父拼图块的信息，用于 TerrainMatching 高度计算
 * （Phase F：子块的 Y 偏移和 groundLevelDelta 依赖父块的投影类型与高度）。
 *
 * freeShape 记录该连接点所属父拼图块的可放置空间（VoxelShape），对应 MC 1.21 PieceState.free。
 * 初始为全局 freeShape（MaxDistance 包围盒减去起始块 AABB）；
 * 若连接点位于父块 AABB 内部，则改为父块 AABB 形状（局部 freeShape，两层级优化）。
 * 子块放置成功后，从 freeShape 减去子块 AABB（ONLY_FIRST），并传给子块的待处理连接点。
 */
struct PendingJoint {
    BlockPos position;      ///< 连接点在世界中的位置
    std::string sourceName; ///< 源连接点名称（Jigsaw 方块的 name 字段）
    std::string targetPool; ///< 目标模板池
    std::string targetType; ///< 目标连接点名称（Jigsaw 方块的 target 字段）
    i32 depth = 0;          ///< 当前深度
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    JigsawOrientation orientation = JigsawOrientation::NorthUp; ///< Jigsaw 方块朝向
    JigsawJointType jointType = JigsawJointType::Rollable;      ///< 连接类型
    i32 placementPriority = 0;                                  ///< 放置优先级（组装队列中降序出队）
    i32 parentMinY = 0;                                         ///< 父拼图块边界框 minY（TerrainMatching 高度计算用）
    i32 parentGroundLevelDelta = 0;                    ///< 父拼图块 groundLevelDelta（TerrainMatching 高度计算用）
    std::shared_ptr<VoxelShape> freeShape;             ///< 父拼图块的可放置空间（VoxelShape 空间追踪）
    structure::StructureBoundingBox parentBoundingBox; ///< 父拼图块边界框（两层级 freeShape 判断用）

    PendingJoint() = default;
    PendingJoint(const BlockPos& pos,
        const std::string& srcName,
        const std::string& pool,
        const std::string& tgtType,
        i32 d,
        JigsawPlacementBehaviour proj = JigsawPlacementBehaviour::Rigid,
        JigsawOrientation orient = JigsawOrientation::NorthUp,
        JigsawJointType jt = JigsawJointType::Rollable,
        i32 placementPrio = 0)
        : position(pos)
        , sourceName(srcName)
        , targetPool(pool)
        , targetType(tgtType)
        , depth(d)
        , projection(proj)
        , orientation(orient)
        , jointType(jt)
        , placementPriority(placementPrio)
    {}

    // 移动构造和移动赋值
    PendingJoint(PendingJoint&& other) noexcept = default;
    PendingJoint& operator=(PendingJoint&& other) noexcept = default;

    // 禁用拷贝
    PendingJoint(const PendingJoint&) = delete;
    PendingJoint& operator=(const PendingJoint&) = delete;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
