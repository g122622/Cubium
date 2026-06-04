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

#include "JigsawManager.hpp"
#include "../../../resource/IResourcePack.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/IWorldWriter.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/block/BlockRegistry.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../feature/template/TemplateLoader.hpp"
#include "../feature/template/TemplateManager.hpp"
#include "JigsawPiece.hpp"

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

// 使用 template_ 命名空间中的类型
using feature::template_::GravityStructureProcessor;
using feature::template_::PlacementSettings;
using feature::template_::StructureProcessorList;
using feature::template_::Template;
using feature::template_::TemplateManager;

// 静态模板管理器实例定义
feature::template_::TemplateManager JigsawManager::s_templateManager;

void JigsawManager::setResourcePack(const IResourcePack* pack)
{
    s_templateManager.setResourcePack(pack);
}

void JigsawManager::clearCache()
{
    s_templateManager.clear();
}

std::vector<PlacedPiece> JigsawManager::assemble(JigsawPatternRegistry& patternRegistry,
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
    Rotation rotation = getRandomRotation(rng);
    Mirror mirror = Mirror::None; // 起始块不使用镜像
    auto boundingBox = calculateBoundingBox(*startPiece, startPos, rotation);

    PlacedPiece startPlaced;
    startPlaced.piece = startPiece->clone();
    startPlaced.position = startPos;
    startPlaced.rotation = rotation;
    startPlaced.mirror = mirror;
    startPlaced.groundLevelDelta = startPiece->getGroundLevelDelta();
    startPlaced.boundingBox = boundingBox;
    startPlaced.joints = _getTransformedJoints(*startPiece, startPos, rotation, mirror);

    placedPieces.push_back(std::move(startPlaced));

    // 将起始块的连接点添加到待处理队列
    // 使用打乱后的连接点顺序
    std::vector<JigsawJoint> shuffledJoints = startPiece->getShuffledJoints(rng);
    for (const auto& joint : shuffledJoints) {
        // 计算旋转后的朝向
        JigsawOrientation rotatedOrientation = JigsawOrientations::rotate(joint.orientation, rotation);

        PendingJoint pending;
        pending.position = transformPosition(joint.sourcePos, rotation, mirror, startPiece->getSize());
        pending.sourceName = joint.sourceName;
        pending.targetPool = joint.targetPool;
        pending.targetType = joint.targetName;
        pending.depth = 0;
        pending.projection = joint.projection;
        pending.orientation = rotatedOrientation;
        pending.jointType = joint.jointType;
        pendingJoints.push(std::move(pending));
    }

    // 处理待处理的连接点
    i32 maxPieces = maxDepth * 20;
    while (!pendingJoints.empty() && static_cast<i32>(placedPieces.size()) < maxPieces) {
        PendingJoint joint = std::move(pendingJoints.front());
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

bool JigsawManager::assembleAndPlace(IWorldWriter& world,
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

        placePieceRecursive(world, placed, rng);
    }

    return true;
}

void JigsawManager::placePieceRecursive(IWorldWriter& world, const PlacedPiece& placed, math::Random& rng)
{
    // 尝试加载模板（如果是 SingleJigsawPiece）
    const SingleJigsawPiece* singlePiece = dynamic_cast<const SingleJigsawPiece*>(placed.piece.get());
    if (singlePiece) {
        const std::string& templateName = singlePiece->getTemplateName();
        if (!templateName.empty()) {
            ResourceLocation templateLoc(templateName);
            const Template* templ = s_templateManager.getTemplate(templateLoc);

            if (templ) {
                // 创建放置设置
                PlacementSettings settings;
                settings.setRotation(placed.rotation);
                settings.setMirror(placed.mirror);

                // 对于 TerrainMatching 放置行为，自动添加 GravityStructureProcessor
                // 添加 GravityStructureProcessor(Heightmap.Type.WORLD_SURFACE_WG, -1)
                // WORLD_SURFACE_WG 对应高度图类型 0（世界表面高度图，用于世界生成）
                StructureProcessorList processorList;
                if (placed.piece->getPlacementBehaviour() == JigsawPlacementBehaviour::TerrainMatching) {
                    // 添加重力处理器，使结构贴合地形
                    // offset = -1 表示将结构底部放在地面以下一格，使其更牢固地嵌入地面
                    processorList.addProcessor(std::make_unique<GravityStructureProcessor>(0, -1));
                }
                settings.setProcessors(&processorList);

                // 如果 world 实现了 IWorld 接口，设置它以便 GravityStructureProcessor 可以查询高度
                const IWorld* iworld = dynamic_cast<const IWorld*>(&world);
                if (iworld) {
                    settings.setWorld(iworld);
                }

                // 放置模板
                templ->place(world, placed.position, settings, rng, 18);
            } else {
                // 模板未找到，创建简单的占位方块
                _placeFallbackBlocks(world, placed, rng);
            }
        }
        return;
    }

    // 处理 ListJigsawPiece（递归放置子块）
    const ListJigsawPiece* listPiece = dynamic_cast<const ListJigsawPiece*>(placed.piece.get());
    if (listPiece) {
        const auto& children = listPiece->getPieces();
        for (const auto& child : children) {
            if (!child) {
                continue;
            }

            // 为每个子块创建 PlacedPiece
            PlacedPiece childPlaced;
            childPlaced.piece = child->clone();
            childPlaced.position = placed.position; // 子块继承父块位置（相对位置在子块内部处理）
            childPlaced.rotation = placed.rotation;
            childPlaced.mirror = placed.mirror;
            childPlaced.groundLevelDelta = child->getGroundLevelDelta();
            childPlaced.boundingBox = calculateBoundingBox(*child, placed.position, placed.rotation);

            placePieceRecursive(world, childPlaced, rng);
        }
        return;
    }

    // 处理其他类型的拼图块（使用回退方块）
    _placeFallbackBlocks(world, placed, rng);
}

void JigsawManager::_placeFallbackBlocks(IWorldWriter& world, const PlacedPiece& placed, math::Random& rng)
{
    // 当模板未找到时，放置简单的方块来标记结构位置
    const BlockState* markerBlock = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);

    if (!markerBlock) {
        markerBlock = VanillaBlocks::getState(VanillaBlocks::STONE);
    }

    if (!markerBlock) {
        return; // 无法获取任何方块
    }

    // 获取边界框并在其中放置方块
    const auto& box = placed.boundingBox;
    for (i32 y = box.minY(); y <= box.maxY(); ++y) {
        for (i32 x = box.minX(); x <= box.maxX(); ++x) {
            for (i32 z = box.minZ(); z <= box.maxZ(); ++z) {
                // 只在边缘放置方块（创建框架）
                if (y == box.minY() || y == box.maxY() || x == box.minX() || x == box.maxX() || z == box.minZ() ||
                    z == box.maxZ()) {
                    // 添加一些随机性，避免过于规则
                    if (rng.nextInt(100) < 80) {
                        world.setBlockState(x, y, z, markerBlock, 18);
                    }
                }
            }
        }
    }
}

std::vector<JigsawJoint> JigsawManager::getTransformedJoints(
    const JigsawPiece& piece, const BlockPos& position, i32 rotation, i32 mirror)
{
    return _getTransformedJoints(piece, position, static_cast<Rotation>(rotation), static_cast<Mirror>(mirror));
}

std::vector<JigsawJoint> JigsawManager::_getTransformedJoints(
    const JigsawPiece& piece, const BlockPos& position, Rotation rotation, Mirror mirror)
{
    std::vector<JigsawJoint> transformed;
    transformed.reserve(piece.getJoints().size());

    BlockPos size = piece.getSize();
    i32 rotationDeg = static_cast<i32>(rotation) * 90;

    for (const auto& joint : piece.getJoints()) {
        JigsawJoint transformedJoint;
        transformedJoint.sourcePos = transformPosition(joint.sourcePos, rotation, mirror, size) + position;
        transformedJoint.sourceName = joint.sourceName;
        transformedJoint.targetPool = joint.targetPool;
        transformedJoint.targetName = joint.targetName;
        transformedJoint.projection = joint.projection;
        transformedJoint.jointType = joint.jointType;

        // 变换 Jigsaw 朝向
        transformedJoint.orientation = JigsawOrientations::rotate(joint.orientation, rotation);
        if (mirror != Mirror::None) {
            transformedJoint.orientation = JigsawOrientations::mirror(transformedJoint.orientation, mirror);
        }

        transformedJoint.sourceGroundY = joint.sourceGroundY;
        transformed.push_back(transformedJoint);
    }

    return transformed;
}

bool JigsawManager::_processJoint(JigsawPatternRegistry& patternRegistry,
    std::vector<PlacedPiece>& placedPieces,
    std::queue<PendingJoint>& pendingJoints,
    const PendingJoint& joint,
    i32 maxDepth,
    math::Random& rng)
{
    return tryPlacePiece(patternRegistry, placedPieces, pendingJoints, joint, maxDepth, rng);
}

bool JigsawManager::tryPlacePiece(JigsawPatternRegistry& patternRegistry,
    std::vector<PlacedPiece>& placedPieces,
    std::queue<PendingJoint>& pendingJoints,
    const PendingJoint& joint,
    i32 maxDepth,
    math::Random& rng)
{
    // 获取目标模板池
    ResourceLocation poolLocation(joint.targetPool);
    const JigsawPattern* targetPool = patternRegistry.getPattern(poolLocation);

    // 如果目标池为空或不存在，尝试使用回退池
    if (!targetPool || targetPool->isEmpty()) {
        return false;
    }

    // 构建候选块列表 - 使用打乱的列表而非多次随机选择
    std::vector<const JigsawPiece*> candidatePieces;

    if (joint.depth < maxDepth) {
        // 添加目标池中打乱后的块
        std::vector<const JigsawPiece*> shuffled = targetPool->getShuffledPieces(rng);
        candidatePieces.insert(candidatePieces.end(), shuffled.begin(), shuffled.end());
    }

    // 添加回退池中打乱后的块
    const ResourceLocation& fallbackLoc = targetPool->getFallback();
    if (!fallbackLoc.path().empty() && fallbackLoc.toString() != "minecraft:empty") {
        const JigsawPattern* fallbackPool = patternRegistry.getPattern(fallbackLoc);
        if (fallbackPool && !fallbackPool->isEmpty()) {
            std::vector<const JigsawPiece*> fallbackShuffled = fallbackPool->getShuffledPieces(rng);
            candidatePieces.insert(candidatePieces.end(), fallbackShuffled.begin(), fallbackShuffled.end());
        }
    }

    if (candidatePieces.empty()) {
        return false;
    }

    // 遍历候选块列表，尝试每个块直到找到合适的
    // 预分配匹配连接点容器（最多有 pieceJoints.size() * 4 个匹配，因为有4种旋转）
    std::vector<std::pair<size_t, Rotation>> matchingJoints;
    matchingJoints.reserve(16); // 预估容量避免循环内重复分配

    for (const JigsawPiece* selectedPiece : candidatePieces) {
        if (!selectedPiece || selectedPiece->isEmpty()) {
            continue;
        }

        // 尝试找到可以匹配的连接点
        const auto& pieceJoints = selectedPiece->getJoints();
        matchingJoints.clear(); // 清空复用已分配的内存

        for (size_t i = 0; i < pieceJoints.size(); ++i) {
            const auto& pieceJoint = pieceJoints[i];
            // 尝试所有旋转
            for (i32 rotDeg = 0; rotDeg < 360; rotDeg += 90) {
                Rotation rotEnum = static_cast<Rotation>(rotDeg / 90);

                // 计算旋转后的朝向
                JigsawOrientation rotatedOrientation = JigsawOrientations::rotate(pieceJoint.orientation, rotEnum);

                // 检查名称和方向是否匹配
                // 匹配条件: source.targetName == target.sourceName && 方向相反
                if (JigsawMatcher::canMatch(pieceJoint.targetName,
                        joint.sourceName,
                        rotatedOrientation,
                        joint.orientation,
                        pieceJoint.jointType)) {
                    matchingJoints.emplace_back(i, rotEnum);
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
        BlockPos jointOffset =
            transformPosition(selectedJoint.sourcePos, rotation, Mirror::None, selectedPiece->getSize());
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
        placed.mirror = Mirror::None;
        placed.groundLevelDelta = selectedPiece->getGroundLevelDelta();
        placed.boundingBox = boundingBox;
        placed.joints = _getTransformedJoints(*selectedPiece, placementPos, rotation, Mirror::None);

        // 创建 JigsawJunction 用于 NoiseChunkGenerator 地形适配
        // JigsawJunction 记录连接点的高度信息，用于后续地形平滑
        i32 sourceGroundY = joint.position.y;
        i32 destGroundY = placementPos.y;
        i32 deltaY = sourceGroundY - destGroundY;

        // 创建 Junction 记录地形适配信息
        // sourceX/sourceZ: 连接点位置
        // sourceGroundY: 源地面高度
        // deltaY: 高度偏移量
        // destProjection: 目标放置行为
        placed.junctions.emplace_back(joint.position.x, // sourceX
            sourceGroundY,                              // sourceGroundY
            joint.position.z,                           // sourceZ
            deltaY,                                     // deltaY
            joint.projection                            // destProjection
        );

        placedPieces.push_back(std::move(placed));

        // 添加新的待处理连接点
        // 使用打乱后的连接点顺序
        std::vector<JigsawJoint> shuffledNewJoints = selectedPiece->getShuffledJoints(rng);
        for (const auto& newJoint : shuffledNewJoints) {
            // 跳过已经匹配的连接点（通过比较位置）
            if (newJoint.sourcePos == selectedJoint.sourcePos) {
                continue;
            }

            // 计算旋转后的朝向
            JigsawOrientation rotatedOrientation = JigsawOrientations::rotate(newJoint.orientation, rotation);

            PendingJoint newPending;
            newPending.position =
                transformPosition(newJoint.sourcePos, rotation, Mirror::None, selectedPiece->getSize()) + placementPos;
            newPending.sourceName = newJoint.sourceName;
            newPending.targetPool = newJoint.targetPool;
            newPending.targetType = newJoint.targetName;
            newPending.depth = joint.depth + 1;
            newPending.projection = newJoint.projection;
            newPending.orientation = rotatedOrientation;
            newPending.jointType = newJoint.jointType;
            pendingJoints.push(std::move(newPending));
        }

        return true;
    }

    return false;
}

structure::StructureBoundingBox JigsawManager::calculateBoundingBox(
    const JigsawPiece& piece, const BlockPos& pos, Rotation rotation)
{
    BlockPos size = piece.getSize();

    // 根据旋转调整尺寸
    if (rotation == Rotation::Clockwise90 || rotation == Rotation::CounterClockwise90) {
        size = BlockPos(size.z, size.y, size.x);
    }

    if (size.x == 0 || size.y == 0 || size.z == 0) {
        return structure::StructureBoundingBox(pos.x, pos.y, pos.z, pos.x, pos.y, pos.z);
    }

    return structure::StructureBoundingBox(
        pos.x, pos.y, pos.z, pos.x + size.x - 1, pos.y + size.y - 1, pos.z + size.z - 1);
}

bool JigsawManager::boxesIntersect(
    const std::vector<PlacedPiece>& placedPieces, const structure::StructureBoundingBox& newBox)
{
    // 使用 0.25 收缩边界进行碰撞检测，收缩边界避免相邻块被判定为重叠
    constexpr f32 SHRINK = 0.25f;
    f32 shrunkMinX = static_cast<f32>(newBox.minX()) + SHRINK;
    f32 shrunkMinY = static_cast<f32>(newBox.minY()) + SHRINK;
    f32 shrunkMinZ = static_cast<f32>(newBox.minZ()) + SHRINK;
    f32 shrunkMaxX = static_cast<f32>(newBox.maxX()) - SHRINK;
    f32 shrunkMaxY = static_cast<f32>(newBox.maxY()) - SHRINK;
    f32 shrunkMaxZ = static_cast<f32>(newBox.maxZ()) - SHRINK;

    for (const auto& placed : placedPieces) {
        const auto& existing = placed.boundingBox;

        // AABB 碰撞检测（使用收缩后的边界）
        if (shrunkMaxX >= static_cast<f32>(existing.minX()) && shrunkMinX <= static_cast<f32>(existing.maxX()) &&
            shrunkMaxY >= static_cast<f32>(existing.minY()) && shrunkMinY <= static_cast<f32>(existing.maxY()) &&
            shrunkMaxZ >= static_cast<f32>(existing.minZ()) && shrunkMinZ <= static_cast<f32>(existing.maxZ())) {
            return true;
        }
    }
    return false;
}

Rotation JigsawManager::getRandomRotation(math::Random& rng)
{
    // 返回 Rotation 枚举值
    return static_cast<Rotation>(rng.nextInt(4));
}

BlockPos JigsawManager::rotatePosition(const BlockPos& pos, Rotation rotation)
{
    switch (rotation) {
        case Rotation::Clockwise90:
            return BlockPos(-pos.z, pos.y, pos.x);
        case Rotation::Clockwise180:
            return BlockPos(-pos.x, pos.y, -pos.z);
        case Rotation::CounterClockwise90:
            return BlockPos(pos.z, pos.y, -pos.x);
        default:
            return pos;
    }
}

BlockPos JigsawManager::mirrorPosition(const BlockPos& pos, Mirror mirror, const BlockPos& center)
{
    BlockPos result = pos;

    switch (mirror) {
        case Mirror::FrontBack: // X 轴镜像
            result = BlockPos(center.x * 2 - pos.x, pos.y, pos.z);
            break;
        case Mirror::LeftRight: // Z 轴镜像
            result = BlockPos(pos.x, pos.y, center.z * 2 - pos.z);
            break;
        default:
            break;
    }

    return result;
}

BlockPos JigsawManager::transformPosition(
    const BlockPos& pos, Rotation rotation, Mirror mirror, const BlockPos& templateSize)
{
    BlockPos result = pos;

    // 先应用镜像（相对于模板中心）
    switch (mirror) {
        case Mirror::FrontBack: // X 轴镜像
            result = BlockPos(templateSize.x - 1 - result.x, result.y, result.z);
            break;
        case Mirror::LeftRight: // Z 轴镜像
            result = BlockPos(result.x, result.y, templateSize.z - 1 - result.z);
            break;
        default:
            break;
    }

    // 然后应用旋转
    switch (rotation) {
        case Rotation::Clockwise90:
            result = BlockPos(templateSize.z - 1 - result.z, result.y, result.x);
            break;
        case Rotation::Clockwise180:
            result = BlockPos(templateSize.x - 1 - result.x, result.y, templateSize.z - 1 - result.z);
            break;
        case Rotation::CounterClockwise90:
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
