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

#include "JigsawAssembler.hpp"

#include "JigsawMatcher.hpp"
#include "JigsawPiece.hpp"
#include "JigsawTransform.hpp"
#include "TemplatePool.hpp"
#include "TemplatePoolRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/physics/shape/BooleanOp.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/AssemblyTypes.hpp"
#include "server/world/gen/jigsaw/JigsawTypes.hpp"
#include "server/world/gen/jigsaw/PoolAliasBinding.hpp"
#include "server/world/gen/jigsaw/PoolAliasLookup.hpp"
#include "server/world/gen/jigsaw/SequencedPriorityIterator.hpp"
#include "server/world/gen/structure/JigsawStructure.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

// 静态模板管理器实例定义（从 JigsawManager 迁移）
feature::template_::TemplateManager JigsawAssembler::s_templateManager;

feature::template_::TemplateManager& JigsawAssembler::getTemplateManager()
{
    return s_templateManager;
}

void JigsawAssembler::clearCache()
{
    // TODO: 目前无调用方（数据包重载尚未实现）；接入资源/数据包热重载时必须调用本方法，
    // 否则重载后仍会命中旧数据包视图下加载的模板缓存。
    s_templateManager.clear();
}

AxisAlignedBB JigsawAssembler::toAabb(const structure::StructureBoundingBox& box)
{
    return AxisAlignedBB(static_cast<f32>(box.minX()),
        static_cast<f32>(box.minY()),
        static_cast<f32>(box.minZ()),
        static_cast<f32>(box.maxX() + 1),
        static_cast<f32>(box.maxY() + 1),
        static_cast<f32>(box.maxZ() + 1));
}

StartPlacement JigsawAssembler::resolveStartPlacement(const JigsawPiece& startPiece,
    const BlockPos& stubPos,
    Rotation rotation,
    IChunkGenerator& generator,
    bool projectStartToHeightmap)
{
    // 先按请求点求起始块包围盒——投影用的列坐标由它导出
    auto boundingBox = JigsawTransform::calculateBoundingBox(startPiece, stubPos, rotation);

    // 【必须用 (minX+maxX)/2 而非包围盒中心】原版 JigsawPlacement 取的是
    // (bb.maxX() + bb.minX()) / 2 与 (bb.maxZ() + bb.minZ()) / 2（整数除法），
    // 而 BoundingBox.getCenter() 是 minX + (maxX-minX+1)/2；跨度与奇偶不同时会相差 1 格，
    // 投影列不同则地形高度不同，整个结构的高度都会跟着偏。
    const i32 centerX = (boundingBox.minX() + boundingBox.maxX()) / 2;
    const i32 centerZ = (boundingBox.minZ() + boundingBox.maxZ()) / 2;

    // project_start_to_heightmap：把该列地形高度叠加到请求点 Y 上，得到目标地面线高度。
    // 不投影时目标地面线就是请求点 Y（即 start_height 给出的绝对高度）。
    const i32 targetGroundLine = projectStartToHeightmap
        ? stubPos.y + generator.getHeight(centerX, centerZ, HeightmapType::WorldSurfaceWG)
        : stubPos.y;

    // 起始块的地面线 = 包围盒 minY + groundLevelDelta；平移使其对齐目标地面线。
    BlockPos origin = stubPos;
    const i32 groundLine = boundingBox.minY() + startPiece.getGroundLevelDelta();
    origin.y += targetGroundLine - groundLine;

    // 候选生成点 = 包围盒 X/Z 中心 + 目标地面线高度，原版据此做生物群系校验
    const BlockPos candidatePoint(centerX, targetGroundLine, centerZ);

    return StartPlacement(
        origin, rotation, JigsawTransform::calculateBoundingBox(startPiece, origin, rotation), candidatePoint);
}

std::vector<PlacedPiece> JigsawAssembler::assemble(TemplatePoolRegistry& poolRegistry,
    const TemplatePool& startPool,
    i32 maxDepth,
    const BlockPos& stubPos,
    bool projectStartToHeightmap,
    bool useExpansionHack,
    math::IRandom& rng,
    IChunkGenerator& generator,
    const PoolAliasBindings* aliases,
    const structure::MaxDistance* maxDistance,
    const structure::DimensionPadding* dimensionPadding)
{
    // 预解析池别名绑定为不可变查找表（对应 MC 1.21 PoolAliasLookup.create）。
    // 无别名时使用空查找表（lookup 恒等映射）。一次性解析保证同一别名多次出现时解析结果一致。
    const PoolAliasLookup aliasLookup = (aliases != nullptr) ? PoolAliasLookup(*aliases, rng) : PoolAliasLookup();

    // 【随机数顺序不可交换】原版先取起始旋转（Rotation.getRandom）再取起始块
    // （StructureTemplatePool.getRandomTemplate），两者都从同一个 WorldgenRandom 抽取；
    // 顺序颠倒会让"旋转"与"模板选择"各拿到对方的随机数，起始块类型和朝向同时错位。
    const Rotation rotation = JigsawTransform::getRandomRotation(rng);
    const JigsawPiece* startPiece = startPool.getRandomPiece(rng);
    if (!startPiece || startPiece->isEmpty()) {
        return {};
    }

    const StartPlacement placement =
        resolveStartPlacement(*startPiece, stubPos, rotation, generator, projectStartToHeightmap);
    return assembleFromStartPlacement(poolRegistry,
        *startPiece,
        placement,
        maxDepth,
        useExpansionHack,
        rng,
        generator,
        aliasLookup,
        maxDistance,
        dimensionPadding);
}

std::vector<Rotation> JigsawAssembler::shuffledRotations(math::IRandom& rng)
{
    // 对应 MC `Rotation.getShuffled(random)` = Util.shuffledCopy(values(), random)：
    // 对 [NONE, CW90, CW180, CCW90] 做一次 Fisher-Yates，消耗 3 次随机数。
    std::vector<Rotation> rotations{
        Rotation::None, Rotation::Clockwise90, Rotation::Clockwise180, Rotation::CounterClockwise90};
    rng.shuffle(rotations);
    return rotations;
}

std::vector<PlacedPiece> JigsawAssembler::assembleFromStartPlacement(TemplatePoolRegistry& poolRegistry,
    const JigsawPiece& startPiece,
    const StartPlacement& placement,
    i32 maxDepth,
    bool useExpansionHack,
    math::IRandom& rng,
    IChunkGenerator& generator,
    const PoolAliasLookup& aliasLookup,
    const structure::MaxDistance* maxDistance,
    const structure::DimensionPadding* dimensionPadding)
{
    const BlockPos& startPos = placement.origin;
    const Rotation rotation = placement.rotation;
    const structure::StructureBoundingBox& boundingBox = placement.boundingBox;

    // 起始块世界高度边界检查（对应 MC 1.21 JigsawPlacement.isStartTooCloseToWorldHeightLimits）
    // 当 DimensionPadding 非 ZERO（top/bottom 至少一个非零）时，若起始块包围盒超出
    // [worldMinY + bottom, worldMinY + getGenDepth() - 1 - top] 则直接返回空列表，
    // 防止结构生成在世界顶/底边界之外。
    // DimensionPadding 为空指针或全零时跳过检查（对应 MC 的 DimensionPadding.ZERO 快速返回 false）。
    if (dimensionPadding != nullptr && (dimensionPadding->top != 0 || dimensionPadding->bottom != 0)) {
        const i32 worldMinY = generator.getMinY();
        const i32 worldMaxYInclusive = worldMinY + generator.getGenDepth() - 1;
        const i32 lowerLimit = worldMinY + dimensionPadding->bottom;
        const i32 upperLimit = worldMaxYInclusive - dimensionPadding->top;
        if (boundingBox.minY() < lowerLimit || boundingBox.maxY() > upperLimit) {
            return {};
        }
    }

    // 【地址稳定性】构件容器用 unique_ptr 持有：组装过程中父构件的 junction 仍会被追加，
    // 队列元素也持有构件指针，按值存放会在 vector 扩容时失效。
    std::vector<std::unique_ptr<PlacedPiece>> pieces;
    auto startPlaced = std::make_unique<PlacedPiece>();
    startPlaced->piece = startPiece.clone();
    startPlaced->position = startPos;
    startPlaced->rotation = rotation;
    startPlaced->mirror = Mirror::None; // 起始块不使用镜像
    startPlaced->groundLevelDelta = startPiece.getGroundLevelDelta();
    startPlaced->projection = startPiece.getPlacementBehaviour();
    startPlaced->boundingBox = boundingBox;
    startPlaced->joints = JigsawTransform::getTransformedJoints(startPiece, startPos, rotation, Mirror::None);
    pieces.push_back(std::move(startPlaced));

    // ===== 初始化可放置空间 freeShape（对应 MC 1.21 JigsawPlacement.addPieces）=====
    // freeShape = MaxDistance 包围盒 - 起始块 AABB（ONLY_FIRST = a && !b）
    // 后续每放置一块即从 freeShape 减去其 AABB，保证不与已放置块重叠、不超出 MaxDistance 范围。
    // maxDistance 缺省时使用 MC 默认值 MaxDistance(80)。
    // 中心点取起始块 AABB 的中心（对应 MC i = (maxX+minX)/2, j = (maxZ+minZ)/2）；
    // Y 中心取 **生成点高度**（MC 的 i1 = k），而非起始块包围盒 minY——起始块被平移对齐地面线后
    // 其 minY = 生成点高度 - groundLevelDelta，两者相差 1，直接用后者会让整个可放置空间下移 1 格。
    //
    // Y 轴裁剪（对应 MC 1.21 JigsawPlacement.addPieces 中的 AABB 构造）：
    //   minY = max(centerY - vertical, worldMinY + padding.bottom)
    //   maxY = min(centerY + vertical + 1, worldMinY + getGenDepth() - padding.top)
    // 其中 worldMinY = generator.getMinY()，worldMinY + getGenDepth() 对应 MC 的 levelMaxY + 1（排他上界）。
    // padding 为空指针时按 DimensionPadding(0, 0) 处理（不裁剪）。
    // 这保证结构不会生成到世界顶/底边界之外。
    const structure::MaxDistance defaultDistance(80);
    const structure::MaxDistance& dist = (maxDistance != nullptr) ? *maxDistance : defaultDistance;
    const i32 centerX = (boundingBox.minX() + boundingBox.maxX()) / 2;
    const i32 centerZ = (boundingBox.minZ() + boundingBox.maxZ()) / 2;
    // 起始块经 Y 旋转不变，故模板原点 Y 加回 groundLevelDelta 即生成点高度 k
    const i32 generationY = startPos.y + startPiece.getGroundLevelDelta();
    const i32 paddingTop = (dimensionPadding != nullptr) ? dimensionPadding->top : 0;
    const i32 paddingBottom = (dimensionPadding != nullptr) ? dimensionPadding->bottom : 0;
    const i32 worldMinY = generator.getMinY();
    const i32 worldMaxExclusive = worldMinY + generator.getGenDepth();
    const i32 clippedMinY = std::max(generationY - dist.vertical, worldMinY + paddingBottom);
    const i32 clippedMaxY = std::min(generationY + dist.vertical + 1, worldMaxExclusive - paddingTop);
    AxisAlignedBB maxDistanceAabb(static_cast<f32>(centerX - dist.horizontal),
        static_cast<f32>(clippedMinY),
        static_cast<f32>(centerZ - dist.horizontal),
        static_cast<f32>(centerX + dist.horizontal + 1),
        static_cast<f32>(clippedMaxY),
        static_cast<f32>(centerZ + dist.horizontal + 1));
    auto globalFreeShape = std::make_shared<VoxelShape>(
        Shapes::join(Shapes::create(maxDistanceAabb), Shapes::create(toAabb(boundingBox)), BooleanOps::OnlyFirst()));

    // 起始块深度为 0；队列元素记录**构件自身**的深度（对应 MC PieceState.depth）
    SequencedPriorityIterator<PendingPiece> pending;
    const PendingPiece startState(pieces.back().get(), globalFreeShape, 0);
    tryPlacingChildren(
        poolRegistry, pieces, pending, startState, aliasLookup, generator, maxDepth, useExpansionHack, rng);

    while (pending.hasNext()) {
        const PendingPiece state = pending.next();
        tryPlacingChildren(
            poolRegistry, pieces, pending, state, aliasLookup, generator, maxDepth, useExpansionHack, rng);
    }

    std::vector<PlacedPiece> result;
    result.reserve(pieces.size());
    for (auto& owned : pieces) {
        result.push_back(std::move(*owned));
    }
    return result;
}

void JigsawAssembler::tryPlacingChildren(TemplatePoolRegistry& poolRegistry,
    std::vector<std::unique_ptr<PlacedPiece>>& pieces,
    SequencedPriorityIterator<PendingPiece>& pending,
    const PendingPiece& state,
    const PoolAliasLookup& aliasLookup,
    IChunkGenerator& generator,
    i32 maxDepth,
    bool useExpansionHack,
    math::IRandom& rng)
{
    PlacedPiece& parent = *state.piece;
    const JigsawPiece& element = *parent.piece;
    const BlockPos& position = parent.position;
    const Rotation rotation = parent.rotation;
    const structure::StructureBoundingBox& parentBox = parent.boundingBox;
    const i32 parentMinY = parentBox.minY();
    const bool isRigidParent = element.getPlacementBehaviour() == JigsawPlacementBehaviour::Rigid;
    const i32 parentGroundLevelDelta = parent.groundLevelDelta;

    // 父块"内部接点"共享的局部可放置空间（对应 MC 的 MutableObject localFree）：
    // 惰性创建一次，在同一父块的所有内部接点之间共享并被逐次扣减。
    std::shared_ptr<VoxelShape> localFree;

    const std::vector<JigsawJoint> parentJoints = element.getShuffledJoints(rng);
    for (size_t jointIndex = 0; jointIndex < parentJoints.size(); ++jointIndex) {
        const JigsawJoint& parentJoint = parentJoints[jointIndex];

        const JigsawOrientation parentOrientation = JigsawOrientations::rotate(parentJoint.orientation, rotation);
        const Direction parentFacing = JigsawOrientations::getFacing(parentOrientation);

        // 父连接点的世界位置与"连接面"（连接点朝向前方一格）
        const BlockPos parentJointPos =
            JigsawTransform::transformPosition(parentJoint.sourcePos, rotation, Mirror::None) + position;
        const BlockPos jointSurface(parentJointPos.x + getStepX(parentFacing),
            parentJointPos.y + getStepY(parentFacing),
            parentJointPos.z + getStepZ(parentFacing));
        // 父连接点相对父块包围盒 minY 的高度（对应 MC 的 j = blockpos1.getY() - i）
        const i32 parentJointRelY = parentJointPos.y - parentMinY;
        // 懒惰求值的地形高度（对应 MC 的 k；尚未求值时为空）
        std::optional<i32> terrainY;

        const TemplatePool* pool = poolRegistry.getPool(aliasLookup.lookup(ResourceLocation(parentJoint.targetPool)));
        const TemplatePool* fallbackPool = (pool != nullptr) ? poolRegistry.getPool(pool->getFallback()) : nullptr;

        // 连接面落在父块内部时使用局部可放置空间（防止结构在自身内部重叠），否则继承父块的空间
        const bool insideParent = parentBox.contains(jointSurface.x, jointSurface.y, jointSurface.z);
        if (insideParent && !localFree) {
            localFree = std::make_shared<VoxelShape>(Shapes::create(toAabb(parentBox)));
        }
        std::shared_ptr<VoxelShape> activeFree = insideParent ? localFree : state.freeShape;

        // 候选元素列表（对应 MC 的 list 构造）：主池（深度未达上限时）+ 回退池。
        // 顺序不可交换——两次 shuffle 都会消耗随机数，且回退池永远追加在末尾。
        std::vector<const JigsawPiece*> candidates;
        if (state.depth != maxDepth && pool != nullptr) {
            const std::vector<const JigsawPiece*> shuffled = pool->getShuffledPieces(rng);
            candidates.insert(candidates.end(), shuffled.begin(), shuffled.end());
        }
        if (fallbackPool != nullptr) {
            const std::vector<const JigsawPiece*> shuffled = fallbackPool->getShuffledPieces(rng);
            candidates.insert(candidates.end(), shuffled.begin(), shuffled.end());
        }

        bool placedForThisJoint = false;
        for (const JigsawPiece* candidate : candidates) {
            // 对应原版 `if (cand == EmptyPoolElement.INSTANCE) break;`——终止整个候选枚举
            if (candidate == nullptr || candidate->isEmpty()) {
                break;
            }

            // 旋转顺序对**每个候选元素**重新洗牌一次（对应 MC `Rotation.getShuffled(random)`）
            const std::vector<Rotation> rotations = shuffledRotations(rng);
            for (const Rotation candidateRotation : rotations) {
                // 候选块的连接点（该旋转下）与包围盒；顺序对每个旋转重新洗牌
                const std::vector<JigsawJoint> candidateJoints = candidate->getShuffledJoints(rng);
                const structure::StructureBoundingBox candidateLocalBox =
                    JigsawTransform::calculateBoundingBox(*candidate, BlockPos(0, 0, 0), candidateRotation);

                const i32 expansion = useExpansionHack
                    ? estimateExpansionHeight(
                          poolRegistry, *candidate, candidateRotation, candidateLocalBox, candidateJoints, aliasLookup)
                    : 0;

                for (const JigsawJoint& candidateJoint : candidateJoints) {
                    // 候选连接点在该旋转下的局部位置与朝向
                    const BlockPos candidateJointPos =
                        JigsawTransform::transformPosition(candidateJoint.sourcePos, candidateRotation, Mirror::None);
                    const JigsawOrientation candidateOrientation =
                        JigsawOrientations::rotate(candidateJoint.orientation, candidateRotation);

                    // 匹配条件：父的 target == 子的 name、正面朝向互为相反、
                    // aligned 时两者 top 朝向一致、joint 类型取父块的
                    if (!JigsawMatcher::canMatch(parentJoint.targetName,
                            candidateJoint.sourceName,
                            parentOrientation,
                            candidateOrientation,
                            parentJoint.jointType)) {
                        continue;
                    }

                    const BlockPos candidateOrigin = jointSurface - candidateJointPos;
                    const i32 childJointLocalY = candidateJointPos.y;
                    const i32 l1 = parentJointRelY - childJointLocalY + getStepY(parentFacing);
                    const bool isRigidChild = candidate->getPlacementBehaviour() == JigsawPlacementBehaviour::Rigid;

                    // 子块基础 Y（对应 MC 的 i2）：双 rigid 走相对几何，否则贴合地形
                    i32 newPieceBaseY = 0;
                    if (isRigidParent && isRigidChild) {
                        newPieceBaseY = parentMinY + l1;
                    } else {
                        if (!terrainY.has_value()) {
                            terrainY =
                                generator.getHeight(parentJointPos.x, parentJointPos.z, HeightmapType::WorldSurfaceWG);
                        }
                        newPieceBaseY = *terrainY - childJointLocalY;
                    }

                    auto candidateBox =
                        JigsawTransform::calculateBoundingBox(*candidate, candidateOrigin, candidateRotation);
                    const i32 yAdjust = newPieceBaseY - candidateBox.minY();
                    const BlockPos candidatePos(candidateOrigin.x, candidateOrigin.y + yAdjust, candidateOrigin.z);
                    if (yAdjust != 0) {
                        candidateBox =
                            JigsawTransform::calculateBoundingBox(*candidate, candidatePos, candidateRotation);
                    }

                    // use_expansion_hack：为矮构件预留竖直净空。撑高后的包围盒**同时**用于碰撞判定
                    // 与构件存储（原版如此），因此它直接决定村庄的 Y 范围。
                    if (expansion > 0) {
                        const i32 grownY = std::max(expansion + 1, candidateBox.maxY() - candidateBox.minY());
                        candidateBox.expandToInclude(
                            candidateBox.minX(), candidateBox.minY() + grownY, candidateBox.minZ());
                    }

                    // 碰撞：候选块收缩 0.25 格后必须完全落在可放置空间内
                    if (Shapes::joinIsNotEmpty(*activeFree,
                            Shapes::create(toAabb(candidateBox).deflate(0.25f)),
                            BooleanOps::OnlySecond())) {
                        continue;
                    }
                    // 放置成功：从可放置空间减去候选块 AABB（未收缩的完整 AABB）
                    *activeFree = Shapes::joinUnoptimized(
                        *activeFree, Shapes::create(toAabb(candidateBox)), BooleanOps::OnlyFirst());

                    const i32 childGroundLevelDelta =
                        isRigidChild ? (parentGroundLevelDelta - l1) : candidate->getGroundLevelDelta();

                    // 连接点的"地面高度"（对应 MC 的 i3 三分支）
                    i32 jointGroundY = 0;
                    if (isRigidParent) {
                        jointGroundY = parentMinY + parentJointRelY;
                    } else if (isRigidChild) {
                        jointGroundY = newPieceBaseY + childJointLocalY;
                    } else {
                        if (!terrainY.has_value()) {
                            terrainY =
                                generator.getHeight(parentJointPos.x, parentJointPos.z, HeightmapType::WorldSurfaceWG);
                        }
                        jointGroundY = *terrainY + l1 / 2;
                    }

                    auto newPiece = std::make_unique<PlacedPiece>();
                    newPiece->piece = candidate->clone();
                    newPiece->position = candidatePos;
                    newPiece->rotation = candidateRotation;
                    newPiece->mirror = Mirror::None;
                    newPiece->groundLevelDelta = childGroundLevelDelta;
                    newPiece->projection = candidate->getPlacementBehaviour();
                    newPiece->boundingBox = candidateBox;
                    newPiece->joints = JigsawTransform::getTransformedJoints(
                        *candidate, candidatePos, candidateRotation, Mirror::None);
                    // 双向 junction：父件侧记子件投影、子件侧记父件投影（对应 MC 的两个 addJunction）
                    parent.junctions.emplace_back(jointSurface.x,
                        jointGroundY - parentJointRelY + parentGroundLevelDelta,
                        jointSurface.z,
                        l1,
                        candidate->getPlacementBehaviour());
                    newPiece->junctions.emplace_back(parentJointPos.x,
                        jointGroundY - childJointLocalY + childGroundLevelDelta,
                        parentJointPos.z,
                        -l1,
                        element.getPlacementBehaviour());

                    PlacedPiece* stored = newPiece.get();
                    pieces.push_back(std::move(newPiece));

                    // 子构件入队条件对应 MC `if (depth + 1 <= this.maxDepth)`；
                    // 优先级取**父连接点**的 placementPriority（对应 MC placing.add(state, l)）
                    if (state.depth + 1 <= maxDepth) {
                        pending.add(PendingPiece(stored, activeFree, state.depth + 1), parentJoint.placementPriority);
                    }

                    placedForThisJoint = true;
                    break; // 对应原版 label129：该连接点已放置，转到父块的下一个连接点
                }
                if (placedForThisJoint) {
                    break;
                }
            }
            if (placedForThisJoint) {
                break;
            }
        }
    }
}

i32 JigsawAssembler::estimateExpansionHeight(TemplatePoolRegistry& poolRegistry,
    const JigsawPiece& piece,
    Rotation rotation,
    const structure::StructureBoundingBox& localBox,
    const std::vector<JigsawJoint>& joints,
    const PoolAliasLookup& aliasLookup)
{
    // 只有"矮"构件才预留竖直净空（原版阈值 16）：高塔类构件自身已经够高，不需要。
    constexpr i32 kMaxCompactYSpan = 16;
    if (localBox.ySpan() > kMaxCompactYSpan) {
        return 0;
    }

    i32 best = 0;
    for (const auto& joint : joints) {
        // 连接面 = 连接点位置 + 朝向步进；只有落在候选块包围盒内部的接点才算"朝内"，
        // 朝外的接点通向结构外部，其高度不受本构件容纳能力约束。
        const BlockPos jointPos = JigsawTransform::transformPosition(joint.sourcePos, rotation, Mirror::None);
        const Direction facing = JigsawOrientations::getFacing(JigsawOrientations::rotate(joint.orientation, rotation));
        const BlockPos surface(
            jointPos.x + getStepX(facing), jointPos.y + getStepY(facing), jointPos.z + getStepZ(facing));
        if (!localBox.contains(surface.x, surface.y, surface.z)) {
            continue;
        }

        const TemplatePool* pool = poolRegistry.getPool(aliasLookup.lookup(ResourceLocation(joint.targetPool)));
        if (pool == nullptr) {
            continue;
        }
        i32 span = pool->getMaxYSpan();
        const TemplatePool* fallback = poolRegistry.getPool(pool->getFallback());
        if (fallback != nullptr) {
            span = std::max(span, fallback->getMaxYSpan());
        }
        best = std::max(best, span);
    }

    return best;
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
