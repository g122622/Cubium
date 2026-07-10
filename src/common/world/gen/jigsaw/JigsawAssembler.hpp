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

#include "AssemblyTypes.hpp"
#include "JigsawTypes.hpp"
#include "PoolAliasBinding.hpp"
#include "PoolAliasLookup.hpp"
#include "SequencedPriorityIterator.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"

#include <memory>
#include <vector>

namespace mc {

class IChunkGenerator;
class VoxelShape;

namespace resource {
class IResourcePack;
} // namespace resource

namespace world {
namespace gen {
namespace structure {
struct MaxDistance;
struct DimensionPadding;
} // namespace structure

namespace feature {
namespace template_ {
class TemplateManager;
} // namespace template_
} // namespace feature

namespace jigsaw {

class TemplatePool;
class TemplatePoolRegistry;

/**
 * @brief Jigsaw 结构组装器
 *
 * 实现递归式结构组装：从起始模板池开始，通过连接点逐步扩展结构。
 * 使用优先级队列（按 placementPriority 降序）替代 FIFO，对应 MC 1.21 的 SequencedPriorityIterator。
 *
 * 空间追踪使用 VoxelShape（freeShape）记录剩余可放置空间，替代简单 AABB 碰撞检测：
 * - 初始 freeShape = MaxDistance 包围盒减去起始块 AABB
 * - 每次放置后从 freeShape 减去新块 AABB（ONLY_FIRST）
 * - 放置前用收缩 0.25 的新块 AABB 检测是否完全在 freeShape 内（ONLY_SECOND）
 *
 * 职责：
 * - 持有静态 TemplateManager（s_templateManager），供 JigsawPiece::loadJointsFromTemplate 等访问
 * - assemble()：BFS 组装，返回 PlacedPiece 列表
 * - tryPlacePiece()：尝试匹配连接点并放置新拼图块
 *
 * 对应 MC 1.21 net.minecraft.world.level.levelgen.structure.pools.JigsawPlacement。
 */
class JigsawAssembler {
public:
    /**
     * @brief 设置资源包（用于加载模板）
     * @param pack 资源包指针
     */
    static void setResourcePack(const resource::IResourcePack* pack);

    /**
     * @brief 获取模板管理器
     * @return 模板管理器引用
     */
    static feature::template_::TemplateManager& getTemplateManager();

    /**
     * @brief 清除模板缓存
     */
    static void clearCache();

    /**
     * @brief 组装结构
     *
     * 从起始模板池开始，通过连接点 BFS 扩展结构，返回所有已放置的拼图块。
     * 使用 VoxelShape 空间追踪限制结构不超出 MaxDistance 范围、不与已放置块重叠。
     *
     * MaxDistance 包围盒的 Y 轴会按 DimensionPadding 与世界高度限制裁剪：
     *   - minY = max(centerY - vertical, generator.getMinY() + padding.bottom)
     *   - maxY = min(centerY + vertical + 1, generator.getMinY() + generator.getGenDepth() - padding.top)
     * 这保证结构不会生成到世界顶/底边界之外。
     *
     * 起始块安全检查：当 dimensionPadding 非空（非 ZERO）且起始块包围盒超出
     * [generator.getMinY() + padding.bottom, generator.getMinY() + getGenDepth() - 1 - padding.top]
     * 时直接返回空列表，避免结构生成在世界边界外。
     *
     * @param poolRegistry 模板池注册表
     * @param startPool 起始模板池
     * @param maxDepth 最大递归深度
     * @param startPos 起始位置
     * @param rng 随机数生成器
     * @param generator 区块生成器（用于 TerrainMatching 投影查询世界表面高度，以及获取世界高度边界）
     * @param aliases 池别名绑定集合（可空，用于试炼密室等结构的池随机化）
     * @param maxDistance 距结构中心的最大距离约束（用于初始化 freeShape 可放置空间）
     * @param dimensionPadding 维度填充（可空，控制结构距世界顶/底边界的最小距离）
     * @return 已放置的拼图块列表
     */
    static std::vector<PlacedPiece> assemble(TemplatePoolRegistry& poolRegistry,
        const TemplatePool& startPool,
        i32 maxDepth,
        const BlockPos& startPos,
        math::Random& rng,
        IChunkGenerator& generator,
        const PoolAliasBindings* aliases = nullptr,
        const structure::MaxDistance* maxDistance = nullptr,
        const structure::DimensionPadding* dimensionPadding = nullptr);

    /**
     * @brief 尝试匹配连接点并放置新拼图块
     *
     * 从目标模板池中选择候选块，尝试匹配连接点，放置成功则加入 placedPieces 并入队新连接点。
     * 使用 freeShape（VoxelShape）进行空间追踪：放置前检测新块是否完全在剩余空间内，
     * 放置后从 freeShape 中减去新块 AABB。
     *
     * freeShape 采用 shared_ptr<VoxelShape> 持有者模型，对应 MC 1.21 的 MutableObject<VoxelShape>：
     *   - 全局 freeShape（连接点在父块外部）：父块与子块共享同一持有者，放置后通过 *holder = ... 更新，
     *     兄弟连接点立即看到更新后的剩余空间。
     *   - 局部 freeShape（连接点在父块内部）：每次 tryPlacePiece 调用惰性创建新持有者，
     *     该次调用内的内部子块共享此持有者，与全局空间隔离。
     *
     * @param poolRegistry 模板池注册表
     * @param placedPieces 已放置的拼图块列表（输出）
     * @param pendingJoints 待处理连接点优先级队列（输出，按 placementPriority 降序出队）
     * @param joint 当前处理的连接点
     * @param aliasLookup 池别名查找表（解析虚拟池名为实际池名）
     * @param generator 区块生成器（用于 TerrainMatching 投影查询世界表面高度）
     * @param maxDepth 最大递归深度
     * @param freeShapeHolder 剩余可放置空间持有者（VoxelShape，会被本方法修改：放置成功后减去新块 AABB）
     * @param rng 随机数生成器
     * @return 是否成功放置
     */
    static bool tryPlacePiece(TemplatePoolRegistry& poolRegistry,
        std::vector<PlacedPiece>& placedPieces,
        SequencedPriorityIterator<PendingJoint>& pendingJoints,
        const PendingJoint& joint,
        const PoolAliasLookup& aliasLookup,
        IChunkGenerator& generator,
        i32 maxDepth,
        const std::shared_ptr<VoxelShape>& freeShapeHolder,
        math::Random& rng);

private:
    /**
     * @brief 从 StructureBoundingBox 构建 AxisAlignedBB（f32 坐标）
     *
     * MC 1.21 使用 AABB.of(BoundingBox)；Cubium 的 StructureBoundingBox 与 BoundingBox 等价。
     * AABB 坐标与 BoundingBox 的 minX..maxZ 一一对应（不扩展 +1）。
     */
    static AxisAlignedBB toAabb(const structure::StructureBoundingBox& box);

    static feature::template_::TemplateManager s_templateManager;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
