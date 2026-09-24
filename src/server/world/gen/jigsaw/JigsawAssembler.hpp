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
#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"

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
 * @brief 起始块的摆放结果
 *
 * 起始块的模板原点、旋转与包围盒。**Y 已完成高度投影对齐**：包围盒的
 * `minY() + groundLevelDelta` 等于目标地面高度（`stubPos.y + 地形高度` 或 `stubPos.y`）。
 * 对应 MC 1.21 JigsawPlacement.addPieces 中 `start.move(0, k - l, 0)` 之后的起点 piece。
 */
struct StartPlacement {
    BlockPos origin;
    Rotation rotation;
    structure::StructureBoundingBox boundingBox;

    /**
     * 候选生成点：包围盒 X/Z 中心 + 地面线高度。
     * 对应原版 `new GenerationStub(new BlockPos(i, i1, j), ...)`，即生物群系校验的采样点。
     */
    BlockPos candidatePoint;

    StartPlacement(const BlockPos& origin_,
        Rotation rotation_,
        const structure::StructureBoundingBox& box,
        const BlockPos& candidatePoint_)
        : origin(origin_)
        , rotation(rotation_)
        , boundingBox(box)
        , candidatePoint(candidatePoint_)
    {}
};

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
 * - tryPlacingChildren()：处理一个构件的全部连接点，逐个匹配并放置子构件
 *
 * 对应 MC 1.21 net.minecraft.world.level.levelgen.structure.pools.JigsawPlacement。
 */
class JigsawAssembler {
public:
    /**
     * @brief 获取模板管理器
     *
     * 返回进程级单例。其模板来源（数据包仓库、结构包资源源）由宿主经
     * MinecraftServer 的 TemplateManagerHostBinding 注入并解绑：单例只持非拥有指针，
     * 严禁绕过宿主绑定直接设置来源，否则宿主销毁后会残留悬垂指针。
     *
     * @return 模板管理器引用
     */
    static feature::template_::TemplateManager& getTemplateManager();

    /**
     * @brief 清除模板缓存
     */
    static void clearCache();

    /**
     * @brief 求出起始块的摆放（模板原点 + 包围盒），并完成 project_start_to_heightmap 的 Y 对齐
     *
     * 对应 MC 1.21 JigsawPlacement.addPieces 的起始段：
     *   1. 先按请求点算出起始块包围盒；
     *   2. 取包围盒的 `(minX + maxX) / 2` 与 `(minZ + maxZ) / 2`（**整数除法，注意与原版
     *      BoundingBox.getCenter() 在跨度为偶数时相差 1**）；
     *   3. 投影时把该列的地形高度加到请求点 Y 上，得到目标地面线高度 k；
     *   4. 把起始块沿 Y 平移，使 `包围盒.minY + groundLevelDelta == k`。
     *
     * @param startPiece 起始拼图块
     * @param stubPos 请求点（区块最小角 + start_height 的 Y）
     * @param rotation 起始块旋转
     * @param generator 区块生成器（投影时查询地形高度）
     * @param projectStartToHeightmap 是否把起始点投影到地形高度图
     * @return 起始块的摆放结果
     */
    [[nodiscard]] static StartPlacement resolveStartPlacement(const JigsawPiece& startPiece,
        const BlockPos& stubPos,
        Rotation rotation,
        IChunkGenerator& generator,
        bool projectStartToHeightmap);

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
     * 【随机数顺序】起始旋转先于起始块选择（对应原版 `Rotation.getRandom(random)` 在
     * `pool.getRandomTemplate(random)` 之前）。顺序颠倒会让起始块的旋转与类型同时错位。
     *
     * @param poolRegistry 模板池注册表
     * @param startPool 起始模板池
     * @param maxDepth 最大递归深度
     * @param stubPos 候选生成点（区块最小角 + start_height 的 Y）
     * @param projectStartToHeightmap 是否把起始点投影到地形高度图
     * @param useExpansionHack 是否启用 use_expansion_hack（为矮构件预留竖直净空）
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
        const BlockPos& stubPos,
        bool projectStartToHeightmap,
        bool useExpansionHack,
        math::IRandom& rng,
        IChunkGenerator& generator,
        const PoolAliasBindings* aliases,
        const structure::MaxDistance* maxDistance,
        const structure::DimensionPadding* dimensionPadding);

    /**
     * @brief 从已确定的起始块继续组装
     *
     * 起始块与旋转已由调用方确定（见 resolveStartPlacement）——把"选起始块"与"生物群系
     * 校验"分开，使校验点落在真实的候选生成点上、且随机数只消耗一次。
     *
     * @param poolRegistry 模板池注册表
     * @param startPiece 起始拼图块
     * @param placement 起始块摆放（模板原点 + 旋转 + 包围盒）
     * @param maxDepth 最大递归深度
     * @param useExpansionHack 是否启用 use_expansion_hack
     * @param rng 随机数生成器
     * @param generator 区块生成器
     * @param aliasLookup 池别名查找表（无别名时传默认构造的空表，对应 PoolAliasLookup.EMPTY）
     * @param maxDistance 距结构中心的最大距离约束（可空则用默认值）
     * @param dimensionPadding 维度填充（可空）
     * @return 已放置的拼图块列表（含起始块）
     */
    static std::vector<PlacedPiece> assembleFromStartPlacement(TemplatePoolRegistry& poolRegistry,
        const JigsawPiece& startPiece,
        const StartPlacement& placement,
        i32 maxDepth,
        bool useExpansionHack,
        math::IRandom& rng,
        IChunkGenerator& generator,
        const PoolAliasLookup& aliasLookup,
        const structure::MaxDistance* maxDistance,
        const structure::DimensionPadding* dimensionPadding);

    /**
     * @brief 处理一个已放置构件的全部连接点，为每个连接点放置至多一个子构件
     *
     * 对应 MC 1.21 JigsawPlacement.Placer.tryPlacingChildren。遍历顺序（**每一步都消耗
     * 世界随机数，顺序不可改动**）：
     *   父构件的连接点（打乱 + 按 selectionPriority 稳定降序）
     *     → 候选元素（主池 + 回退池，各自按权重展开后打乱）
     *       → 旋转（四个旋转顺序打乱）
     *         → 该候选在该旋转下的连接点（打乱）
     *           → 名称/朝向匹配、定位、碰撞检测；**命中即放置，该连接点处理完毕**
     *
     * 与"收集全部匹配再随机挑一个"的写法相比，本形态有两个实质性差别：
     *   1. 随机数消耗模式与原版一致（每个候选元素消耗 3 次旋转洗牌 + (m-1) 次连接点洗牌）；
     *   2. 某个候选/旋转被碰撞否决时会继续尝试其余候选与旋转，而不是放弃整个连接点。
     *
     * 连接点位于父块包围盒内部时使用**本调用内共享**的局部可放置空间（对应 MC 的
     * MutableObject localFree，惰性创建、被父块各内部接点逐次扣减）；位于外部时继承
     * 父块的可放置空间。
     *
     * @param poolRegistry 模板池注册表
     * @param pieces 已放置构件容器（拥有所有权，地址稳定，供队列引用）
     * @param pending 待处理构件优先级队列（输出，按 placementPriority 降序出队）
     * @param state 当前处理的父构件（构件 + 继承的可放置空间 + 深度）
     * @param aliasLookup 池别名查找表（解析虚拟池名为实际池名）
     * @param generator 区块生成器（TerrainMatching 与高度投影时查询世界表面高度）
     * @param maxDepth 最大递归深度
     * @param useExpansionHack 是否启用 use_expansion_hack
     * @param rng 随机数生成器
     */
    static void tryPlacingChildren(TemplatePoolRegistry& poolRegistry,
        std::vector<std::unique_ptr<PlacedPiece>>& pieces,
        SequencedPriorityIterator<PendingPiece>& pending,
        const PendingPiece& state,
        const PoolAliasLookup& aliasLookup,
        IChunkGenerator& generator,
        i32 maxDepth,
        bool useExpansionHack,
        math::IRandom& rng);

private:
    /**
     * @brief use_expansion_hack 的净空估算：候选块还能从自身"朝内接点"长出多高
     *
     * 对应 MC 1.21 JigsawPlacement.tryPlacingChildren 中计算 i1 的 stream：
     * 候选块旋转后 Y 跨度 > 16 时返回 0（矮构件才需要预留净空）；
     * 否则遍历其连接点，只统计**连接面落在候选块包围盒内部**（朝内）的连接点，
     * 取其所引用池与回退池的最大 Y 跨度的最大值。该值本身不消耗随机数
     * （原版 getMaxSize 同样是确定性求值并缓存）。
     *
     * @param poolRegistry 模板池注册表
     * @param piece 候选拼图块
     * @param rotation 候选块的旋转
     * @param localBox 候选块在原点 + 该旋转下的包围盒（由调用方复用，避免重复计算）
     * @param joints 候选块在**该旋转下**的连接点（已打乱；顺序不影响取最大值）
     * @param aliasLookup 池别名查找表
     * @return 预估净空高度；无需扩张时返回 0
     */
    [[nodiscard]] static i32 estimateExpansionHeight(TemplatePoolRegistry& poolRegistry,
        const JigsawPiece& piece,
        Rotation rotation,
        const structure::StructureBoundingBox& localBox,
        const std::vector<JigsawJoint>& joints,
        const PoolAliasLookup& aliasLookup);

    /**
     * @brief 取四个旋转的打乱顺序（对应 MC `Rotation.getShuffled(random)`，消耗 3 次随机数）
     *
     * 原版对**每个候选元素**都重新洗牌一次旋转顺序——不是每个连接点、也不是全局一次。
     */
    [[nodiscard]] static std::vector<Rotation> shuffledRotations(math::IRandom& rng);

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
