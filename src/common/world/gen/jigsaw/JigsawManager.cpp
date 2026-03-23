#include "JigsawManager.hpp"
#include "JigsawPiece.hpp"
#include "../feature/template/TemplateManager.hpp"
#include "../feature/template/TemplateLoader.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/block/BlockRegistry.hpp"

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

// 使用 template_ 命名空间中的类型
using feature::template_::Template;
using feature::template_::PlacementSettings;
using feature::template_::TemplateManager;

// 静态模板管理器实例
static TemplateManager s_templateManager;

std::vector<PlacedPiece> JigsawManager::assemble(
    JigsawPatternRegistry& patternRegistry,
    const JigsawPattern& startPool,
    i32 maxDepth,
    const BlockPos& startPos,
    math::Random& rng)
{
    std::vector<PlacedPiece> placedPieces;
    std::queue<PendingJoint> pendingJoints;

    // 从起始模板池选择起始块
    const JigsawPiece* startPiece = startPool.getRandomPiece(rng);
    if (!startPiece || startPiece->isEmpty()) {
        return placedPieces;
    }

    // 放置起始块
    i32 rotation = getRandomRotation(rng);
    i32 mirror = 0;  // 起始块不使用镜像
    auto boundingBox = calculateBoundingBox(*startPiece, startPos, rotation);

    PlacedPiece startPlaced;
    startPlaced.piece = startPiece->clone();
    startPlaced.position = startPos;
    startPlaced.rotation = rotation;
    startPlaced.mirror = mirror;
    startPlaced.groundLevelDelta = startPiece->getGroundLevelDelta();
    startPlaced.boundingBox = boundingBox;
    startPlaced.joints = getTransformedJoints(*startPiece, startPos, rotation, mirror);

    placedPieces.push_back(std::move(startPlaced));

    // 将起始块的连接点添加到待处理队列
    for (const auto& joint : startPiece->getJoints()) {
        PendingJoint pending;
        pending.position = transformPosition(joint.sourcePos, rotation, mirror, startPiece->getSize());
        pending.sourceName = joint.sourceName;
        pending.targetPool = joint.targetPool;
        pending.targetType = joint.targetName;
        pending.depth = 0;
        pending.projection = joint.projection;
        pendingJoints.push(pending);
    }

    // 处理待处理的连接点
    i32 maxPieces = maxDepth * 20;
    while (!pendingJoints.empty() && static_cast<i32>(placedPieces.size()) < maxPieces) {
        PendingJoint joint = pendingJoints.front();
        pendingJoints.pop();

        if (joint.depth >= maxDepth) {
            continue;
        }

        // 检查目标模板池
        if (joint.targetPool.empty() || joint.targetPool == "minecraft:empty") {
            continue;
        }

        tryPlacePiece(patternRegistry, placedPieces, pendingJoints, joint, maxDepth, rng);
    }

    return placedPieces;
}

bool JigsawManager::assembleAndPlace(
    IWorldWriter& world,
    JigsawPatternRegistry& patternRegistry,
    const JigsawPattern& startPool,
    i32 maxDepth,
    const BlockPos& startPos,
    math::Random& rng)
{
    auto placedPieces = assemble(patternRegistry, startPool, maxDepth, startPos, rng);

    if (placedPieces.empty()) {
        return false;
    }

    // 放置每个拼图块
    for (const auto& placed : placedPieces) {
        if (!placed.piece) {
            continue;
        }

        // 尝试加载模板（如果是 SingleJigsawPiece）
        const SingleJigsawPiece* singlePiece = dynamic_cast<const SingleJigsawPiece*>(placed.piece.get());
        if (singlePiece) {
            const String& templateName = singlePiece->getTemplateName();
            if (!templateName.empty()) {
                ResourceLocation templateLoc(templateName);
                const Template* templ = s_templateManager.getTemplate(templateLoc);

                if (templ) {
                    // 创建放置设置
                    PlacementSettings settings;
                    settings.setRotation(placed.rotation);
                    settings.setMirror(placed.mirror);

                    // 放置模板
                    templ->place(world, placed.position, settings, rng, 18);
                }
            }
        }

        // TODO: 处理 ListJigsawPiece（递归放置子块）
    }

    return true;
}

std::vector<JigsawJoint> JigsawManager::getTransformedJoints(
    const JigsawPiece& piece,
    const BlockPos& position,
    i32 rotation,
    i32 mirror)
{
    std::vector<JigsawJoint> transformed;
    transformed.reserve(piece.getJoints().size());

    BlockPos size = piece.getSize();

    for (const auto& joint : piece.getJoints()) {
        JigsawJoint transformedJoint;
        transformedJoint.sourcePos = transformPosition(joint.sourcePos, rotation, mirror, size) + position;
        transformedJoint.sourceName = JigsawMatcher::rotateName(joint.sourceName, rotation);
        transformedJoint.targetPool = joint.targetPool;
        transformedJoint.targetName = joint.targetName;
        transformedJoint.projection = joint.projection;
        transformedJoint.sourceGroundY = joint.sourceGroundY;
        transformed.push_back(transformedJoint);
    }

    return transformed;
}

bool JigsawManager::processJoint(
    JigsawPatternRegistry& patternRegistry,
    std::vector<PlacedPiece>& placedPieces,
    std::queue<PendingJoint>& pendingJoints,
    const PendingJoint& joint,
    i32 maxDepth,
    math::Random& rng)
{
    return tryPlacePiece(patternRegistry, placedPieces, pendingJoints, joint, maxDepth, rng);
}

bool JigsawManager::tryPlacePiece(
    JigsawPatternRegistry& patternRegistry,
    std::vector<PlacedPiece>& placedPieces,
    std::queue<PendingJoint>& pendingJoints,
    const PendingJoint& joint,
    i32 maxDepth,
    math::Random& rng)
{
    // 获取目标模板池
    ResourceLocation poolLocation(joint.targetPool);
    const JigsawPattern* targetPool = patternRegistry.getPattern(poolLocation);

    if (!targetPool || targetPool->isEmpty()) {
        return false;
    }

    // 尝试多次找到一个合适的块
    constexpr i32 maxAttempts = 20;
    for (i32 attempt = 0; attempt < maxAttempts; ++attempt) {
        const JigsawPiece* selectedPiece = targetPool->getRandomPiece(rng);
        if (!selectedPiece || selectedPiece->isEmpty()) {
            continue;
        }

        // 尝试找到可以匹配的连接点
        const auto& pieceJoints = selectedPiece->getJoints();
        std::vector<std::pair<size_t, i32>> matchingJoints;

        for (size_t i = 0; i < pieceJoints.size(); ++i) {
            const auto& pieceJoint = pieceJoints[i];
            // 检查连接点是否可以匹配
            if (JigsawMatcher::canMatch(pieceJoint.sourceName, joint.sourceName)) {
                // 尝试所有旋转
                for (i32 rot = 0; rot < 360; rot += 90) {
                    String rotatedName = JigsawMatcher::rotateName(pieceJoint.sourceName, rot);
                    if (JigsawMatcher::canMatch(rotatedName, joint.sourceName)) {
                        matchingJoints.emplace_back(i, rot);
                    }
                }
            }
        }

        if (matchingJoints.empty()) {
            continue;
        }

        // 随机选择一个匹配
        auto [jointIndex, rotation] = matchingJoints[rng.nextInt(static_cast<i32>(matchingJoints.size()))];
        const auto& selectedJoint = pieceJoints[jointIndex];

        // 计算放置位置
        // 连接点的位置需要使两个块连接在一起
        BlockPos jointOffset = transformPosition(selectedJoint.sourcePos, rotation, 0, selectedPiece->getSize());
        BlockPos placementPos = joint.position - jointOffset;

        // 计算边界框
        auto boundingBox = calculateBoundingBox(*selectedPiece, placementPos, rotation);

        // 检查是否与已放置的块重叠
        if (boxesIntersect(placedPieces, boundingBox)) {
            continue;
        }

        // 创建已放置的块
        PlacedPiece placed;
        placed.piece = selectedPiece->clone();
        placed.position = placementPos;
        placed.rotation = rotation;
        placed.mirror = 0;
        placed.groundLevelDelta = selectedPiece->getGroundLevelDelta();
        placed.boundingBox = boundingBox;
        placed.joints = getTransformedJoints(*selectedPiece, placementPos, rotation, 0);

        placedPieces.push_back(std::move(placed));

        // 添加新的待处理连接点
        for (const auto& newJoint : selectedPiece->getJoints()) {
            // 跳过已经匹配的连接点
            if (&newJoint == &selectedJoint) {
                continue;
            }

            PendingJoint newPending;
            newPending.position = transformPosition(newJoint.sourcePos, rotation, 0, selectedPiece->getSize()) + placementPos;
            newPending.sourceName = JigsawMatcher::rotateName(newJoint.sourceName, rotation);
            newPending.targetPool = newJoint.targetPool;
            newPending.targetType = newJoint.targetName;
            newPending.depth = joint.depth + 1;
            newPending.projection = newJoint.projection;
            pendingJoints.push(newPending);
        }

        return true;
    }

    return false;
}

structure::StructureBoundingBox JigsawManager::calculateBoundingBox(
    const JigsawPiece& piece,
    const BlockPos& pos,
    i32 rotation)
{
    BlockPos size = piece.getSize();

    // 根据旋转调整尺寸
    if (rotation == 90 || rotation == 270) {
        size = BlockPos(size.z, size.y, size.x);
    }

    if (size.x == 0 || size.y == 0 || size.z == 0) {
        return structure::StructureBoundingBox(pos.x, pos.y, pos.z, pos.x, pos.y, pos.z);
    }

    return structure::StructureBoundingBox(
        pos.x, pos.y, pos.z,
        pos.x + size.x - 1, pos.y + size.y - 1, pos.z + size.z - 1
    );
}

bool JigsawManager::boxesIntersect(
    const std::vector<PlacedPiece>& placedPieces,
    const structure::StructureBoundingBox& newBox)
{
    for (const auto& placed : placedPieces) {
        const auto& existing = placed.boundingBox;

        // AABB 碰撞检测
        if (newBox.maxX() >= existing.minX() && newBox.minX() <= existing.maxX() &&
            newBox.maxY() >= existing.minY() && newBox.minY() <= existing.maxY() &&
            newBox.maxZ() >= existing.minZ() && newBox.minZ() <= existing.maxZ()) {
            return true;
        }
    }
    return false;
}

i32 JigsawManager::getRandomRotation(math::Random& rng) {
    // 返回 0, 90, 180, 或 270 度
    return rng.nextInt(4) * 90;
}

BlockPos JigsawManager::rotatePosition(const BlockPos& pos, i32 rotation) {
    switch (rotation) {
        case 90:
            return BlockPos(-pos.z, pos.y, pos.x);
        case 180:
            return BlockPos(-pos.x, pos.y, -pos.z);
        case 270:
            return BlockPos(pos.z, pos.y, -pos.x);
        default:
            return pos;
    }
}

BlockPos JigsawManager::mirrorPosition(const BlockPos& pos, i32 mirror, const BlockPos& center) {
    BlockPos result = pos;

    if (mirror == 1) {  // X 轴镜像
        result = BlockPos(center.x * 2 - pos.x, pos.y, pos.z);
    } else if (mirror == 2) {  // Z 轴镜像
        result = BlockPos(pos.x, pos.y, center.z * 2 - pos.z);
    }

    return result;
}

BlockPos JigsawManager::transformPosition(
    const BlockPos& pos,
    i32 rotation,
    i32 mirror,
    const BlockPos& templateSize)
{
    BlockPos result = pos;

    // 先应用镜像（相对于模板中心）
    if (mirror == 1) {  // X 轴镜像
        result = BlockPos(templateSize.x - 1 - result.x, result.y, result.z);
    } else if (mirror == 2) {  // Z 轴镜像
        result = BlockPos(result.x, result.y, templateSize.z - 1 - result.z);
    }

    // 然后应用旋转
    switch (rotation) {
        case 90:
            result = BlockPos(templateSize.z - 1 - result.z, result.y, result.x);
            break;
        case 180:
            result = BlockPos(templateSize.x - 1 - result.x, result.y, templateSize.z - 1 - result.z);
            break;
        case 270:
            result = BlockPos(result.z, result.y, templateSize.x - 1 - result.x);
            break;
        default:
            break;
    }

    return result;
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
