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
 * @brief 待处理的构件（组装队列元素）
 *
 * 对应 MC 1.21 JigsawPlacement.Placer.PieceState。队列以**构件**而非连接点为单位：
 * 原版一次处理一个父构件的**全部**连接点，并在其中共享一个"父块内部"的局部可放置空间
 * （localFree），因此队列元素必须携带父构件本身，而不是一个孤立的连接点。
 *
 * piece 指向已放置构件；地址稳定性由调用方的 `std::vector<std::unique_ptr<PlacedPiece>>`
 * 保证（构件在校验/放置过程中仍会被追加 junction，不能按值搬移）。
 *
 * freeShape 是该构件继承的可放置空间（对应 MC PieceState.free）：构件放置成功时，
 * 把它的包围盒从父块的可放置空间中减去，并把结果持有者传给本元素。
 * 本构件自身的接点若落在其包围盒**内部**，则改用临时的局部持有者，与此处无关。
 *
 * depth 是**本构件自身**的深度（起始块为 0）。原版据此决定：能否从主池取候选
 * （`depth != maxDepth`）、子构件能否入队（`depth + 1 <= maxDepth`）。
 */
struct PendingPiece {
    PlacedPiece* piece;                    ///< 已放置构件（非拥有，地址稳定）
    std::shared_ptr<VoxelShape> freeShape; ///< 该构件继承的可放置空间（VoxelShape 空间追踪）
    i32 depth;                             ///< 该构件自身的深度

    PendingPiece(PlacedPiece* piece_, std::shared_ptr<VoxelShape> freeShape_, i32 depth_)
        : piece(piece_)
        , freeShape(std::move(freeShape_))
        , depth(depth_)
    {}

    // 移动构造和移动赋值
    PendingPiece(PendingPiece&& other) noexcept = default;
    PendingPiece& operator=(PendingPiece&& other) noexcept = default;

    // 禁用拷贝
    PendingPiece(const PendingPiece&) = delete;
    PendingPiece& operator=(const PendingPiece&) = delete;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
