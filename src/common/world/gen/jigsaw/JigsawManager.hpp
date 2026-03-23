#pragma once

#include "JigsawPiece.hpp"
#include "JigsawPattern.hpp"
#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../structure/StructureBoundingBox.hpp"
#include "../feature/template/Template.hpp"
#include <vector>
#include <queue>
#include <memory>

namespace mc {

class IWorldWriter;

namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 已放置的拼图块信息
 */
struct PlacedPiece {
    std::unique_ptr<JigsawPiece> piece;
    BlockPos position;
    i32 rotation = 0;
    i32 mirror = 0;  // 0=none, 1=x, 2=z
    i32 groundLevelDelta = 0;
    structure::StructureBoundingBox boundingBox;
    std::vector<JigsawJoint> joints;  ///< 已变换的连接点

    PlacedPiece() = default;
    PlacedPiece(std::unique_ptr<JigsawPiece> p, const BlockPos& pos, i32 rot, i32 mir, i32 delta, const structure::StructureBoundingBox& box)
        : piece(std::move(p)), position(pos), rotation(rot), mirror(mir), groundLevelDelta(delta), boundingBox(box) {}
};

/**
 * @brief 待处理的连接点
 */
struct PendingJoint {
    BlockPos position;              ///< 连接点在世界中的位置
    String sourceName;              ///< 源连接点名称
    String targetPool;              ///< 目标模板池
    String targetType;              ///< 目标连接点名称
    i32 depth = 0;                  ///< 当前深度
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;

    PendingJoint() = default;
    PendingJoint(const BlockPos& pos, const String& srcName, const String& pool, const String& tgtType, i32 d, JigsawPlacementBehaviour proj = JigsawPlacementBehaviour::Rigid)
        : position(pos), sourceName(srcName), targetPool(pool), targetType(tgtType), depth(d), projection(proj) {}
};

/**
 * @brief Jigsaw 结构组装器
 *
 * 参考 MC 1.16.5 的 JigsawManager，实现递归式结构组装。
 * 从起始模板池开始，通过连接点逐步扩展结构。
 */
class JigsawManager {
public:
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
    static std::vector<PlacedPiece> assemble(
        JigsawPatternRegistry& patternRegistry,
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
    static bool assembleAndPlace(
        IWorldWriter& world,
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
        const JigsawPiece& piece,
        const BlockPos& position,
        i32 rotation,
        i32 mirror);

private:
    /**
     * @brief 处理单个连接点
     */
    static bool processJoint(
        JigsawPatternRegistry& patternRegistry,
        std::vector<PlacedPiece>& placedPieces,
        std::queue<PendingJoint>& pendingJoints,
        const PendingJoint& joint,
        i32 maxDepth,
        math::Random& rng);

    /**
     * @brief 计算拼图块的边界框
     */
    static structure::StructureBoundingBox calculateBoundingBox(
        const JigsawPiece& piece,
        const BlockPos& pos,
        i32 rotation);

    /**
     * @brief 检查边界框是否重叠
     */
    static bool boxesIntersect(
        const std::vector<PlacedPiece>& placedPieces,
        const structure::StructureBoundingBox& newBox);

    /**
     * @brief 获取随机旋转角度
     */
    static i32 getRandomRotation(math::Random& rng);

    /**
     * @brief 应用旋转变换到位置
     */
    static BlockPos rotatePosition(const BlockPos& pos, i32 rotation);

    /**
     * @brief 应用镜像变换到位置
     */
    static BlockPos mirrorPosition(const BlockPos& pos, i32 mirror, const BlockPos& center);

    /**
     * @brief 变换连接点位置
     */
    static BlockPos transformPosition(
        const BlockPos& pos,
        i32 rotation,
        i32 mirror,
        const BlockPos& templateSize);

    /**
     * @brief 尝试匹配和放置新拼图块
     */
    static bool tryPlacePiece(
        JigsawPatternRegistry& patternRegistry,
        std::vector<PlacedPiece>& placedPieces,
        std::queue<PendingJoint>& pendingJoints,
        const PendingJoint& joint,
        i32 maxDepth,
        math::Random& rng);
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
