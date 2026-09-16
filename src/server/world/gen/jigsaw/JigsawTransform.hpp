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

#include "JigsawTypes.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"

#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

class JigsawPiece;

/**
 * @brief Jigsaw 坐标/连接点变换工具
 *
 * 从 JigsawManager 提取的纯工具函数，负责拼图块坐标变换、连接点变换、边界框计算和随机旋转。
 * 对应 MC 1.21 中 JigsawJigsawManager 的变换相关静态方法。
 */
class JigsawTransform {
public:
    /**
     * @brief 变换连接点位置（镜像 + 旋转）
     *
     * 对拼图块内的局部坐标应用镜像和旋转，转换为模板局部坐标系中的新坐标。
     *
     * @param pos 原始位置（模板局部坐标）
     * @param rotation 旋转角度
     * @param mirror 镜像模式
     * @param templateSize 模板尺寸（用于镜像中心计算）
     * @return 变换后的位置
     */
    static BlockPos transformPosition(
        const BlockPos& pos, Rotation rotation, Mirror mirror, const BlockPos& templateSize);

    /**
     * @brief 获取已变换的连接点列表
     *
     * 对拼图块的所有连接点应用位置和朝向变换，返回世界坐标系中的连接点列表。
     *
     * @param piece 拼图块
     * @param position 放置位置（世界坐标）
     * @param rotation 旋转角度
     * @param mirror 镜像模式
     * @return 变换后的连接点列表
     */
    static std::vector<JigsawJoint> getTransformedJoints(
        const JigsawPiece& piece, const BlockPos& position, Rotation rotation, Mirror mirror);

    /**
     * @brief 计算拼图块的边界框
     *
     * @param piece 拼图块
     * @param pos 放置位置
     * @param rotation 旋转角度
     * @return 边界框
     */
    static structure::StructureBoundingBox calculateBoundingBox(
        const JigsawPiece& piece, const BlockPos& pos, Rotation rotation);

    /**
     * @brief 获取随机旋转角度
     * @param rng 随机数生成器
     * @return 旋转角度（None/Clockwise90/Clockwise180/CounterClockwise90 之一）
     */
    static Rotation getRandomRotation(math::Random& rng);
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
