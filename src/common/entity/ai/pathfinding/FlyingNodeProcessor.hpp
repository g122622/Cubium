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

#include "PathNodeType.hpp"
#include "PathPoint.hpp"
#include "WalkNodeProcessor.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 飞行实体的路径节点处理器
 *
 * 对齐 vanilla `FlyNodeEvaluator`。与地面寻路（`WalkNodeProcessor`）的核心差异：
 *
 *  1. **邻居展开是 26 方向立体移动**，而不是地面版的 8 方向水平 + 跳跃/跌落。
 *     飞行实体可以朝任意方向直线飞行，不存在"必须踩在方块上"的约束。
 *  2. **节点类型判定放宽**：原版 `FlyNodeEvaluator.getPathType` 把脚下的非危险方块
 *     一律折算成 WALKABLE / OPEN，因为飞行实体不依赖地面支撑；只有火、岩浆、
 *     仙人掌这类会持续造成伤害的方块才保留危险语义。
 *  3. **起点取实体所在高度**（`floor(y + 0.5)`），而不是像地面版那样向下找地面。
 *     飞行实体悬空时，脚下的"地面"可能在几十格外，用它当起点会让整条路径从
 *     错误的高度出发。
 *
 * 对角/斜向移动的合法性判断对齐原版：走某条斜边时，要求组成这条边的两条直角边
 * 本身也可通行（`hasMalus`）。否则飞机会"贴着墙角穿过去"。
 *
 * 使用方式（对齐 vanilla `FlyingPathNavigation`）：
 * ```cpp
 * auto processor = std::make_unique<FlyingNodeProcessor>();
 * auto finder = std::make_unique<PathFinder>(std::move(processor));
 * auto navigator = std::make_unique<PathNavigator>(std::move(finder));
 * navigator->setEntity(mob);
 * ```
 *
 * TODO: 原版 `FlyingPathNavigation` 还附带"最大下落高度"与"落地判定"（`canFloat` 分支
 *   让落水的飞行实体沿水面寻路）。本项目尚未实现落水漂浮分支，飞行实体进水后仍按
 *   空中逻辑寻路。待需要时补 `setCanFloat`。
 */
class FlyingNodeProcessor : public WalkNodeProcessor {
public:
    FlyingNodeProcessor() = default;
    ~FlyingNodeProcessor() override = default;

    // ========== 起点 ==========

    /**
     * @brief 获取飞行实体的起始节点
     *
     * 对齐 vanilla `FlyNodeEvaluator.getStart`：取 `floor(y + 0.5)` 作为起点高度，
     * 而不是像地面版那样向下搜索地面。
     *
     * 若该位置本身被判定为不可通行（例如实体半个身子埋在方块里），依次尝试
     * 包围盒四角对应的候选点，取第一个可通行的；全都不可通行时仍返回原起点，
     * 让上层按"无路径"处理，而不是崩溃或返回空指针。
     */
    [[nodiscard]] PathPoint* getStartNode(i32 x, i32 y, i32 z) override;

    // ========== 邻居展开 ==========

    /**
     * @brief 展开 26 方向的立体邻居
     *
     * 对齐 vanilla `FlyNodeEvaluator.getNeighbors`：
     *  - 先取 6 个正交方向；
     *  - 再取 12 个面内斜向（要求两条直角边都可通行）；
     *  - 最后取 8 个体对角（要求相关三条边都可通行）。
     *
     * 每个候选位置经 `_findAcceptedNode` 过滤：节点类型必须是可飞行通过的，
     * 且代价惩罚 >= 0（负值表示该方块类型对此实体完全不可通行）。
     */
    [[nodiscard]] std::vector<PathPoint*> getNeighbors(PathPoint* current) override;

    // ========== 节点类型 ==========

    /**
     * @brief 飞行实体的节点类型判定
     *
     * 对齐 vanilla `FlyNodeEvaluator.getPathType`：
     *  - 本格是 OPEN 且不是世界最低层时，看**下方一格**的方块类型：
     *    火焰/岩浆 → DAMAGE_FIRE，其他伤害方块 → DAMAGE_OTHER，可可果 → COCOA，
     *    栅栏 → FENCE（但实体自己脚下那格除外，否则实体永远无法起飞），
     *    其余非 WALKABLE/OPEN/WATER 的方块折算成 WALKABLE（飞行实体的"可站立"），
     *    否则保持 OPEN；
     *  - 结果为 WALKABLE 或 OPEN 时，再检查四邻是否贴着危险方块
     *    （对齐 `checkNeighbourBlocks`，避免飞行实体紧贴岩浆飞行被烧）。
     */
    [[nodiscard]] PathNodeType getNodeType(i32 x, i32 y, i32 z) override;

    /**
     * @brief 带实体尺寸的节点类型判定
     *
     * 飞行实体体积更大时，仅检查中心格会漏判"贴着天花板的角落"。这里在
     * `getNodeType` 的结果上叠加一层：若中心格可通行，但实体包围盒覆盖到的
     * 相邻格里有不可通行的，按不可通行处理。
     */
    [[nodiscard]] PathNodeType getNodeTypeWithEntity(i32 x, i32 y, i32 z) override;

    // ========== 缓存 ==========

    /**
     * @brief 清空节点缓存
     *
     * 地面版的判定完全按坐标查询方块，不需要额外状态；飞行版额外缓存了"按位置
     * 查询节点类型"的结果（原版 `pathTypeByPosCache`）。一次寻路会话开始时必须
     * 清掉，否则上一次会话的判定会串到这一次。
     */
    void clear() override;

protected:
    [[nodiscard]] std::unique_ptr<PathPoint> createNode(i32 x, i32 y, i32 z) override;

private:
    /**
     * @brief 按坐标查询节点类型（带会话内缓存）
     *
     * 对应原版 `FlyNodeEvaluator.getCachedPathType`。同一次寻路里同一个坐标会被
     * 反复查询（26 方向展开 + 斜向合法性检查），缓存能把方块查询次数压下来。
     */
    [[nodiscard]] PathNodeType _cachedNodeType(i32 x, i32 y, i32 z);

    /**
     * @brief 该实体的寻路惩罚是否允许通过该类型
     *
     * 惩罚为负表示"此类型对该实体完全不可通行"（如蜜蜂对水设置了 -1）。
     * 对应原版 `mob.getPathfindingMalus(type) >= 0`。
     */
    [[nodiscard]] bool _isPassableForMob(PathNodeType type) const;

    /**
     * @brief 取一个可接受的邻居节点
     *
     * 对应原版 `FlyNodeEvaluator.findAcceptedNode`：类型不可通行返回 nullptr；
     * 否则取/建节点，写入类型与代价。
     */
    [[nodiscard]] PathPoint* _findAcceptedNode(i32 x, i32 y, i32 z);

    /**
     * @brief 该节点是否存在且代价有效（对应原版 `hasMalus`）
     *
     * 用于斜向移动的直角边合法性判断。
     */
    [[nodiscard]] static bool _hasMalus(const PathPoint* node);

    /**
     * @brief 检查四邻是否贴着危险方块（对应原版 `checkNeighbourBlocks`）
     *
     * 飞行实体贴着岩浆/火焰飞行时会被点燃，因此即使本格是空气，只要相邻格
     * 是危险方块，就把本格标成对应的 DAMAGE_* 类型，让寻路主动绕开。
     */
    [[nodiscard]] PathNodeType _checkNeighbourBlocks(i32 x, i32 y, i32 z, PathNodeType type);

    /// 会话内"坐标 → 节点类型"缓存（键由 makeHash 生成，与 NodeProcessor 节点缓存同构）。
    std::unordered_map<u64, PathNodeType> m_pathTypeByPosCache;
};

} // namespace mc::entity::ai::pathfinding
