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
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/AssemblyTypes.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include "common/world/gen/jigsaw/PoolAliasBinding.hpp"
#include "common/world/gen/jigsaw/PoolAliasLookup.hpp"
#include "common/world/gen/jigsaw/SequencedPriorityIterator.hpp"
#include "common/world/gen/structure/JigsawStructure.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

// 静态模板管理器实例定义（从 JigsawManager 迁移）
feature::template_::TemplateManager JigsawAssembler::s_templateManager;

void JigsawAssembler::setResourcePack(const resource::IResourcePack* pack)
{
    s_templateManager.setResourcePack(pack);
}

feature::template_::TemplateManager& JigsawAssembler::getTemplateManager()
{
    return s_templateManager;
}

void JigsawAssembler::clearCache()
{
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

std::vector<PlacedPiece> JigsawAssembler::assemble(TemplatePoolRegistry& poolRegistry,
    const TemplatePool& startPool,
    i32 maxDepth,
    const BlockPos& startPos,
    math::Random& rng,
    IChunkGenerator& generator,
    const PoolAliasBindings* aliases,
    const structure::MaxDistance* maxDistance,
    const structure::DimensionPadding* dimensionPadding)
{
    std::vector<PlacedPiece> placedPieces;
    // 按优先级降序出队的待处理连接点队列（对应 MC 1.21 JigsawPlacement.Placer.placing）。
    // 连接点按其 placementPriority 入队，高优先级先出队；同优先级内按入队顺序出队（FIFO）。
    SequencedPriorityIterator<PendingJoint> pendingJoints;

    // 预解析池别名绑定为不可变查找表（对应 MC 1.21 PoolAliasLookup.create）。
    // 无别名时使用空查找表（lookup 恒等映射）。一次性解析保证同一别名多次出现时解析结果一致。
    PoolAliasLookup aliasLookup = (aliases != nullptr) ? PoolAliasLookup(*aliases, rng) : PoolAliasLookup();

    // 从起始模板池选择起始块
    const JigsawPiece* startPiece = startPool.getRandomPiece(rng);
    if (!startPiece || startPiece->isEmpty()) {
        return placedPieces;
    }

    // 放置起始块
    Rotation rotation = JigsawTransform::getRandomRotation(rng);
    Mirror mirror = Mirror::None; // 起始块不使用镜像
    auto boundingBox = JigsawTransform::calculateBoundingBox(*startPiece, startPos, rotation);

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
            return placedPieces;
        }
    }

    PlacedPiece startPlaced;
    startPlaced.piece = startPiece->clone();
    startPlaced.position = startPos;
    startPlaced.rotation = rotation;
    startPlaced.mirror = mirror;
    startPlaced.groundLevelDelta = startPiece->getGroundLevelDelta();
    startPlaced.projection = startPiece->getPlacementBehaviour();
    startPlaced.boundingBox = boundingBox;
    startPlaced.joints = JigsawTransform::getTransformedJoints(*startPiece, startPos, rotation, mirror);

    placedPieces.push_back(std::move(startPlaced));

    // ===== 初始化可放置空间 freeShape（对应 MC 1.21 JigsawPlacement.addPieces）=====
    // freeShape = MaxDistance 包围盒 - 起始块 AABB（ONLY_FIRST = a && !b）
    // 后续每放置一块即从 freeShape 减去其 AABB，保证不与已放置块重叠、不超出 MaxDistance 范围。
    // maxDistance 缺省时使用 MC 默认值 MaxDistance(80)。
    // 中心点取起始块 AABB 的中心（对应 MC i = (maxX+minX)/2, j = (maxZ+minZ)/2, i1 = startPos.y）
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
    const i32 centerY = startPos.y;
    const i32 paddingTop = (dimensionPadding != nullptr) ? dimensionPadding->top : 0;
    const i32 paddingBottom = (dimensionPadding != nullptr) ? dimensionPadding->bottom : 0;
    const i32 worldMinY = generator.getMinY();
    const i32 worldMaxExclusive = worldMinY + generator.getGenDepth();
    const i32 clippedMinY = std::max(centerY - dist.vertical, worldMinY + paddingBottom);
    const i32 clippedMaxY = std::min(centerY + dist.vertical + 1, worldMaxExclusive - paddingTop);
    AxisAlignedBB maxDistanceAabb(static_cast<f32>(centerX - dist.horizontal),
        static_cast<f32>(clippedMinY),
        static_cast<f32>(centerZ - dist.horizontal),
        static_cast<f32>(centerX + dist.horizontal + 1),
        static_cast<f32>(clippedMaxY),
        static_cast<f32>(centerZ + dist.horizontal + 1));
    VoxelShape globalFreeShape =
        Shapes::join(Shapes::create(maxDistanceAabb), Shapes::create(toAabb(boundingBox)), BooleanOps::OnlyFirst());

    // 将起始块的连接点添加到待处理队列
    // 使用打乱后的连接点顺序（getShuffledJoints 内部按 selectionPriority 降序稳定排序）
    std::vector<JigsawJoint> shuffledJoints = startPiece->getShuffledJoints(rng);
    for (const auto& joint : shuffledJoints) {
        // 计算旋转后的朝向
        JigsawOrientation rotatedOrientation = JigsawOrientations::rotate(joint.orientation, rotation);

        PendingJoint pending;
        pending.position =
            JigsawTransform::transformPosition(joint.sourcePos, rotation, mirror, startPiece->getSize()) + startPos;
        pending.sourceName = joint.sourceName;
        pending.targetPool = joint.targetPool;
        pending.targetType = joint.targetName;
        pending.depth = 0;
        pending.projection = joint.projection;
        pending.orientation = rotatedOrientation;
        pending.jointType = joint.jointType;
        pending.placementPriority = joint.placementPriority;
        // 记录父块信息用于 TerrainMatching 高度计算（起始块以自身为父）
        pending.parentMinY = boundingBox.minY();
        pending.parentGroundLevelDelta = startPiece->getGroundLevelDelta();
        // 起始块的连接点继承全局 freeShape 和起始块边界框
        pending.freeShape = std::make_shared<VoxelShape>(globalFreeShape);
        pending.parentBoundingBox = boundingBox;
        // 入队时按 placementPriority 分桶（高优先级先出队）
        pendingJoints.add(std::move(pending), joint.placementPriority);
    }

    // 处理待处理的连接点
    // VoxelShape 空间追踪保证不重叠、不越界，无需 maxPieces 硬编码上限（对应 MC 1.21 仅用 maxDepth + freeShape 限制）。
    while (pendingJoints.hasNext()) {
        PendingJoint joint = pendingJoints.next();

        if (joint.depth >= maxDepth) {
            continue;
        }

        // 检查目标模板池
        if (joint.targetPool.empty() || joint.targetPool == "minecraft:empty") {
            continue;
        }

        tryPlacePiece(
            poolRegistry, placedPieces, pendingJoints, joint, aliasLookup, generator, maxDepth, joint.freeShape, rng);
    }

    return placedPieces;
}

bool JigsawAssembler::tryPlacePiece(TemplatePoolRegistry& poolRegistry,
    std::vector<PlacedPiece>& placedPieces,
    SequencedPriorityIterator<PendingJoint>& pendingJoints,
    const PendingJoint& joint,
    const PoolAliasLookup& aliasLookup,
    IChunkGenerator& generator,
    i32 maxDepth,
    const std::shared_ptr<VoxelShape>& freeShapeHolder,
    math::Random& rng)
{
    // 获取目标模板池（先经池别名查找表解析虚拟池名，对应 MC 1.21 resourcekey = aliasLookup.lookup(pool)）
    ResourceLocation poolLocation(joint.targetPool);
    const ResourceLocation& resolvedPool = aliasLookup.lookup(poolLocation);
    const TemplatePool* targetPool = poolRegistry.getPool(resolvedPool);

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
        const TemplatePool* fallbackPool = poolRegistry.getPool(fallbackLoc);
        if (fallbackPool && !fallbackPool->isEmpty()) {
            std::vector<const JigsawPiece*> fallbackShuffled = fallbackPool->getShuffledPieces(rng);
            candidatePieces.insert(candidatePieces.end(), fallbackShuffled.begin(), fallbackShuffled.end());
        }
    }

    if (candidatePieces.empty()) {
        return false;
    }

    // ===== 两层级 freeShape（对应 MC 1.21 JigsawPlacement.tryPlacingChildren 的 mutableobject 逻辑）=====
    // flag1 = 父块边界框是否包含连接面（blockpos2 = jigsaw 方块前方一格）。
    //   若包含：使用局部 freeShape（父块 AABB 形状），子块只能在父块内部放置。
    //   否则：使用全局 freeShape（freeShapeHolder），子块在父块外部放置。
    // 局部 freeShape 延迟初始化（对应 MC mutableobject.get() == null 时
    // setValue(Shapes.create(AABB.of(boundingbox)))）。
    //
    // 采用 shared_ptr<VoxelShape> 持有者模型（对应 MC MutableObject<VoxelShape>）：
    //   - 全局空间：freeShapeHolder 在父块与所有外部子块间共享，放置后通过 *holder = ... 更新，
    //     兄弟连接点立即看到更新后的剩余空间（与 MC mutableobject1 = p_227266_ 一致）。
    //   - 局部空间：localHolder 本次调用惰性创建，内部子块共享此持有者（与 MC mutableobject 一致）。
    const Direction parentFacing = JigsawOrientations::getFacing(joint.orientation);
    const BlockPos connectionSurface(joint.position.x + getStepX(parentFacing),
        joint.position.y + getStepY(parentFacing),
        joint.position.z + getStepZ(parentFacing));
    const bool childJointInsideParent =
        joint.parentBoundingBox.contains(connectionSurface.x, connectionSurface.y, connectionSurface.z);
    // 局部 freeShape 持有者（延迟初始化为父块 AABB 形状），对应 MC 的 mutableobject。
    std::shared_ptr<VoxelShape> localHolder;

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

        // 计算放置位置（刚性连接：连接点对齐）
        // jointOffset = 子块连接点在子块局部坐标系中（旋转后）的位置
        // placementPos = 父连接点世界位置 - jointOffset（使两连接点对齐）
        // 对应 MC 1.21: blockpos4 = blockpos2.subtract(blockpos3)
        BlockPos jointOffset = JigsawTransform::transformPosition(
            selectedJoint.sourcePos, rotation, Mirror::None, selectedPiece->getSize());
        BlockPos placementPos = joint.position - jointOffset;

        // 计算边界框
        auto boundingBox = JigsawTransform::calculateBoundingBox(*selectedPiece, placementPos, rotation);

        // ===== TerrainMatching 高度计算（对应 MC 1.21 JigsawPlacement.tryPlacingChildren）=====
        // MC 变量映射：
        //   i   = 父块边界框 minY（joint.parentMinY）
        //   j   = 父 jigsaw 方块 Y 相对父块 minY = blockpos1.getY() - i（parentJointRelY）
        //   k1  = 子 jigsaw 方块局部 Y（selectedJoint.sourcePos.y = childJointLocalY）
        //   l1  = j - k1 + facing.getStepY()（连接点的 deltaY，含朝向 Y 步进）
        //   i2  = newPieceBaseY（子块基础 Y）
        //   j2  = yAdjust = i2 - boundingbox2.minY()
        //   l2  = childGroundLevelDelta
        //   flag  = 父块投影 == RIGID（isRigidParent）
        //   flag2 = 子块投影 == RIGID（isRigidChild）
        //
        // joint.position 是父 jigsaw 方块的世界坐标（非连接面坐标），对应 MC 的 blockpos1。
        // 因此 parentJointRelY = joint.position.y - joint.parentMinY 等价于 MC 的 j = blockpos1.getY() - i。
        // facingStepY 对应 MC 的 direction.getStepY()：垂直连接为 ±1，水平连接为 0。
        const i32 parentJointRelY = joint.position.y - joint.parentMinY;
        const i32 childJointLocalY = selectedJoint.sourcePos.y;
        const i32 facingStepY = getStepY(parentFacing);
        const i32 l1 = parentJointRelY - childJointLocalY + facingStepY;

        const bool isRigidParent = (joint.projection == JigsawPlacementBehaviour::Rigid);
        const bool isRigidChild = (selectedPiece->getPlacementBehaviour() == JigsawPlacementBehaviour::Rigid);

        // newPieceBaseY（i2）：子块基础 Y
        //   RIGID + RIGID：父块 minY + l1（相对父块放置）
        //   否则：世界表面高度 - childJointLocalY（贴合地形）
        i32 newPieceBaseY;
        if (isRigidParent && isRigidChild) {
            newPieceBaseY = joint.parentMinY + l1;
        } else {
            const i32 surfaceY = generator.getHeight(joint.position.x, joint.position.z, HeightmapType::WorldSurfaceWG);
            newPieceBaseY = surfaceY - childJointLocalY;
        }

        // yAdjust（j2）：将子块放置位置从连接点对齐位置移动到 newPieceBaseY
        const i32 yAdjust = newPieceBaseY - boundingBox.minY();
        if (yAdjust != 0) {
            placementPos.y += yAdjust;
            boundingBox = JigsawTransform::calculateBoundingBox(*selectedPiece, placementPos, rotation);
        }

        // ===== VoxelShape 空间追踪（对应 MC 1.21 tryPlacingChildren 的 freeShape 检查）=====
        // 选择当前连接点使用的 freeShape 持有者（两层级）：
        //   childJointInsideParent → 局部持有者（惰性初始化为父块 AABB 形状，对应 MC mutableobject）
        //   否则 → 全局持有者（freeShapeHolder，从父块继承，对应 MC p_227266_）
        // 持有者选择对应 MC：mutableobject1 = flag1 ? mutableobject : p_227266_
        if (childJointInsideParent && !localHolder) {
            localHolder = std::make_shared<VoxelShape>(Shapes::create(toAabb(joint.parentBoundingBox)));
        }
        const std::shared_ptr<VoxelShape>& activeHolder = childJointInsideParent ? localHolder : freeShapeHolder;
        const VoxelShape& activeFreeShape = *activeHolder;

        // 检查子块（收缩 0.25 格后）是否完全在 activeFreeShape 内：
        //   joinIsNotEmpty(activeFreeShape, deflatedNewAABB, ONLY_SECOND) == true 表示子块有部分不在 freeShape 内
        //   （即与已占用空间相交或超出 MaxDistance），跳过该候选块。
        //   对应 MC: !Shapes.joinIsNotEmpty(mutableobject1.get(), Shapes.create(AABB.of(boundingbox3).deflate(0.25)),
        //   BooleanOp.ONLY_SECOND)
        const AxisAlignedBB deflatedNewAabb = toAabb(boundingBox).deflate(0.25f);
        if (Shapes::joinIsNotEmpty(activeFreeShape, Shapes::create(deflatedNewAabb), BooleanOps::OnlySecond())) {
            continue;
        }

        // groundLevelDelta（l2）：
        //   RIGID 子块：parentGroundLevelDelta - l1
        //   TERRAIN_MATCHING 子块：selectedPiece->getGroundLevelDelta()
        const i32 childGroundLevelDelta =
            isRigidChild ? (joint.parentGroundLevelDelta - l1) : selectedPiece->getGroundLevelDelta();

        // ===== 放置成功：从 activeFreeShape 减去子块 AABB（对应 MC mutableobject1.setValue(joinUnoptimized(...,
        // ONLY_FIRST))）===== 注意：减去的是未收缩的完整 AABB（对应 MC 的 AABB.of(boundingbox3)，非 deflate(0.25)）。
        // 通过 *activeHolder = ... 更新持有者，所有共享该持有者的兄弟连接点立即看到更新后的剩余空间。
        *activeHolder =
            Shapes::joinUnoptimized(activeFreeShape, Shapes::create(toAabb(boundingBox)), BooleanOps::OnlyFirst());

        // 创建已放置的块
        PlacedPiece placed;
        placed.piece = selectedPiece->clone();
        placed.position = placementPos;
        placed.rotation = rotation;
        placed.mirror = Mirror::None;
        placed.groundLevelDelta = childGroundLevelDelta;
        placed.projection = selectedPiece->getPlacementBehaviour();
        placed.boundingBox = boundingBox;
        placed.joints = JigsawTransform::getTransformedJoints(*selectedPiece, placementPos, rotation, Mirror::None);

        // 创建 JigsawJunction 用于 NoiseChunkGenerator 地形适配
        // JigsawJunction 记录连接点的高度信息，用于后续地形平滑
        i32 sourceGroundY = joint.position.y;
        i32 destGroundY = placementPos.y;
        i32 deltaY = sourceGroundY - destGroundY;

        placed.junctions.emplace_back(joint.position.x, // sourceX
            sourceGroundY,                              // sourceGroundY
            joint.position.z,                           // sourceZ
            deltaY,                                     // deltaY
            joint.projection                            // destProjection
        );

        placedPieces.push_back(std::move(placed));

        // 子块继承放置后的 freeShape 持有者（shared_ptr 共享，对应 MC PieceState.free = mutableobject1）。
        // 子连接点位于子块内部时使用局部持有者（惰性创建为子块 AABB），否则共享此持有者。
        // 共享同一 shared_ptr 而非拷贝，确保兄弟子块放置时通过 *holder = ... 更新彼此可见。
        const std::shared_ptr<VoxelShape>& childFreeShapeHolder = activeHolder;

        // 添加新的待处理连接点
        // 使用打乱后的连接点顺序（getShuffledJoints 内部按 selectionPriority 降序稳定排序）
        std::vector<JigsawJoint> shuffledNewJoints = selectedPiece->getShuffledJoints(rng);
        for (const auto& newJoint : shuffledNewJoints) {
            // 跳过已经匹配的连接点（通过比较位置）
            if (newJoint.sourcePos == selectedJoint.sourcePos) {
                continue;
            }

            // 计算旋转后的朝向
            JigsawOrientation rotatedOrientation = JigsawOrientations::rotate(newJoint.orientation, rotation);

            PendingJoint newPending;
            newPending.position = JigsawTransform::transformPosition(
                                      newJoint.sourcePos, rotation, Mirror::None, selectedPiece->getSize()) +
                placementPos;
            newPending.sourceName = newJoint.sourceName;
            newPending.targetPool = newJoint.targetPool;
            newPending.targetType = newJoint.targetName;
            newPending.depth = joint.depth + 1;
            newPending.projection = newJoint.projection;
            newPending.orientation = rotatedOrientation;
            newPending.jointType = newJoint.jointType;
            newPending.placementPriority = newJoint.placementPriority;
            // 记录父块信息用于子块的 TerrainMatching 高度计算
            newPending.parentMinY = boundingBox.minY();
            newPending.parentGroundLevelDelta = childGroundLevelDelta;
            // 子块继承放置后的 freeShape 持有者（对应 MC PieceState.free = mutableobject1）
            newPending.freeShape = childFreeShapeHolder;
            newPending.parentBoundingBox = boundingBox;
            // 子连接点以其自身的 placementPriority 入队（高优先级先出队，对应 MC Placer.placing.add(state, l)）
            pendingJoints.add(std::move(newPending), newJoint.placementPriority);
        }

        return true;
    }

    return false;
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
