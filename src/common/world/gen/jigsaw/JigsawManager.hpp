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

#include "../../../core/Types.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../feature/template/Template.hpp"
#include "../structure/StructureBoundingBox.hpp"
#include "JigsawJunction.hpp"
#include "JigsawPattern.hpp"
#include "JigsawPiece.hpp"
#include <memory>
#include <queue>
#include <vector>

namespace mc {

class IWorldWriter;
class IResourcePack;

namespace world {
namespace gen {
namespace feature {
namespace template_ {
class TemplateManager;
}
} // namespace feature
} // namespace gen
} // namespace world

namespace world {
namespace gen {
namespace jigsaw {

// 使用 Direction.hpp 中定义的 Rotation 和 Mirror 枚举
using mc::Mirror;
using mc::Rotation;

/**
 * @brief 已放置的拼图块信息
 */
struct PlacedPiece {
    std::unique_ptr<JigsawPiece> piece;
    BlockPos position;
    Rotation rotation = Rotation::None;
    Mirror mirror = Mirror::None;
    i32 groundLevelDelta = 0;
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
 */
struct PendingJoint {
    BlockPos position;      ///< 连接点在世界中的位置
    std::string sourceName; ///< 源连接点名称（Jigsaw方块的name字段）
    std::string targetPool; ///< 目标模板池
    std::string targetType; ///< 目标连接点名称（Jigsaw方块的target字段）
    i32 depth = 0;          ///< 当前深度
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    JigsawOrientation orientation = JigsawOrientation::NorthUp; ///< Jigsaw 方块朝向
    JigsawJointType jointType = JigsawJointType::Rollable;      ///< 连接类型

    PendingJoint() = default;
    PendingJoint(const BlockPos& pos,
        const std::string& srcName,
        const std::string& pool,
        const std::string& tgtType,
        i32 d,
        JigsawPlacementBehaviour proj = JigsawPlacementBehaviour::Rigid,
        JigsawOrientation orient = JigsawOrientation::NorthUp,
        JigsawJointType jt = JigsawJointType::Rollable)
        : position(pos)
        , sourceName(srcName)
        , targetPool(pool)
        , targetType(tgtType)
        , depth(d)
        , projection(proj)
        , orientation(orient)
        , jointType(jt)
    {}

    // 移动构造和移动赋值
    PendingJoint(PendingJoint&& other) noexcept = default;
    PendingJoint& operator=(PendingJoint&& other) noexcept = default;

    // 禁用拷贝
    PendingJoint(const PendingJoint&) = delete;
    PendingJoint& operator=(const PendingJoint&) = delete;
};

/**
 * @brief Jigsaw 结构组装器
 *
 * 实现递归式结构组装，从起始模板池开始，通过连接点逐步扩展结构。
 */
class JigsawManager {
public:
    /**
     * @brief 设置资源包（用于加载模板）
     * @param pack 资源包指针
     */
    static void setResourcePack(const IResourcePack* pack);

    /**
     * @brief 获取模板管理器
     * @return 模板管理器引用
     */
    static feature::template_::TemplateManager& getTemplateManager() { return s_templateManager; }

    /**
     * @brief 组装结构
     *
     * @param patternRegistry 模板池注册表
     * @param startPool 起始模板池
     * @param maxDepth 最大递归深度
     * @param startPos 起始位置
     * @param rng 随机数生成器
     * @return std::vector<PlacedPiece> 已放置的拼图块列表
     */
    static std::vector<PlacedPiece> assemble(JigsawPatternRegistry& patternRegistry,
        const JigsawPattern& startPool,
        i32 maxDepth,
        const BlockPos& startPos,
        math::Random& rng);

    /**
     * @brief 组装结构并生成到世界
     *
     * @param world 世界写入器
     * @param patternRegistry 模板池注册表
     * @param startPool 起始模板池
     * @param maxDepth 最大递归深度
     * @param startPos 起始位置
     * @param rng 随机数生成器
     * @return bool 是否成功生成
     */
    static bool assembleAndPlace(IWorldWriter& world,
        JigsawPatternRegistry& patternRegistry,
        const JigsawPattern& startPool,
        i32 maxDepth,
        const BlockPos& startPos,
        math::Random& rng);

    /**
     * @brief 获取已放置拼图块的变换后连接点
     *
     * @param piece 拼图块
     * @param position 放置位置
     * @param rotation 旋转角度
     * @param mirror 镜像模式
     * @return std::vector<JigsawJoint> 变换后的连接点列表
     */
    static std::vector<JigsawJoint> getTransformedJoints(
        const JigsawPiece& piece, const BlockPos& position, i32 rotation, i32 mirror);

    /**
     * @brief 清除模板缓存
     */
    static void clearCache();

    /**
     * @brief 计算拼图块的边界框
     * @param piece 拼图块
     * @param pos 放置位置
     * @param rotation 旋转角度
     * @return 边界框
     */
    static structure::StructureBoundingBox calculateBoundingBox(
        const JigsawPiece& piece, const BlockPos& pos, Rotation rotation);

    /**
     * @brief 检查边界框是否与已放置的拼图块重叠
     * @param placedPieces 已放置的拼图块列表
     * @param newBox 新边界框
     * @return 是否重叠
     */
    static bool boxesIntersect(
        const std::vector<PlacedPiece>& placedPieces, const structure::StructureBoundingBox& newBox);

    /**
     * @brief 获取随机旋转角度
     * @param rng 随机数生成器
     * @return 旋转角度
     */
    static Rotation getRandomRotation(math::Random& rng);

    /**
     * @brief 应用旋转变换到位置
     * @param pos 原始位置
     * @param rotation 旋转角度
     * @return 旋转后的位置
     */
    static BlockPos rotatePosition(const BlockPos& pos, Rotation rotation);

    /**
     * @brief 应用镜像变换到位置
     * @param pos 原始位置
     * @param mirror 镜像模式
     * @param center 中心点
     * @return 镜像后的位置
     */
    static BlockPos mirrorPosition(const BlockPos& pos, Mirror mirror, const BlockPos& center);

    /**
     * @brief 变换连接点位置
     * @param pos 原始位置
     * @param rotation 旋转角度
     * @param mirror 镜像模式
     * @param templateSize 模板尺寸
     * @return 变换后的位置
     */
    static BlockPos transformPosition(
        const BlockPos& pos, Rotation rotation, Mirror mirror, const BlockPos& templateSize);

    /**
     * @brief 尝试匹配和放置新拼图块
     */
    static bool tryPlacePiece(JigsawPatternRegistry& patternRegistry,
        std::vector<PlacedPiece>& placedPieces,
        std::queue<PendingJoint>& pendingJoints,
        const PendingJoint& joint,
        i32 maxDepth,
        math::Random& rng);

    /**
     * @brief 递归放置拼图块
     *
     * 将已组装的 PlacedPiece 放置到世界中。
     * 此方法公开以便结构生成器可以手动控制放置过程。
     *
     * @param world 世界写入器
     * @param placed 已放置的拼图块信息
     * @param rng 随机数生成器
     */
    static void placePieceRecursive(IWorldWriter& world, const PlacedPiece& placed, math::Random& rng);

private:
    /**
     * @brief 放置回退方块（当模板未找到时）
     * @param world 世界写入器
     * @param placed 已放置的拼图块信息
     * @param rng 随机数生成器
     */
    static void _placeFallbackBlocks(IWorldWriter& world, const PlacedPiece& placed, math::Random& rng);

    /**
     * @brief 处理单个连接点
     */
    static bool _processJoint(JigsawPatternRegistry& patternRegistry,
        std::vector<PlacedPiece>& placedPieces,
        std::queue<PendingJoint>& pendingJoints,
        const PendingJoint& joint,
        i32 maxDepth,
        math::Random& rng);

    /**
     * @brief 获取变换后的连接点
     */
    static std::vector<JigsawJoint> _getTransformedJoints(
        const JigsawPiece& piece, const BlockPos& pos, Rotation rotation, Mirror mirror);

    static feature::template_::TemplateManager s_templateManager;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
