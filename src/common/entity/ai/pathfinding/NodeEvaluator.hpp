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

/**
 * @file NodeEvaluator.hpp
 * @brief 寻路求值器基类（对应原版 `NodeEvaluator`）。
 *
 * 职责边界：
 *   - **本类**：维护一次寻路会话内的节点缓存、实体尺寸推导、能力开关（能否开门 / 游泳 /
 *     跨越栅栏）、以及把「世界区域」与「实体」绑定成一次会话；
 *   - **子类**：决定某格是什么类型（`getPathType`）、起点在哪（`getStart`）、
 *     有哪些邻居（`getNeighbors`）。
 *
 * 会话生命周期严格对应原版：
 *   `prepare(region, mob)` → 若干次 `getPathType` / `getNode` / `getNeighbors` → `done()`
 *
 * 节点缓存属于会话：`done()` 之后缓存里的节点指针全部失效，因此 `Path` 必须持有节点
 * 的**拷贝**，而不能持有指针。
 */

#pragma once

#include "PathPoint.hpp"
#include "PathType.hpp"
#include "Region.hpp"
#include "Target.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

class MobEntity;

namespace entity::ai::pathfinding {

/**
 * @brief 寻路实体契约
 *
 * 求值器只通过这些读法了解实体：尺寸、跳跃高度、能否开门 / 游泳、以及每种路径类型
 * 对它是否可走（代价）。刻意不收窄成对整个 `MobEntity` 的依赖：
 * 差分探针要能用同一个求值器喂入**受控的**实体参数，从而把「求值器算得对不对」
 * 与「某个具体生物配置得对不对」分开验证。
 *
 * 生产实现就是 `MobEntity` 自己（它直接继承并在 `MobEntity.hpp` 里实现这些纯虚函数）。
 */
class PathfindingEntity {
public:
    virtual ~PathfindingEntity() = default;

    /// 碰撞箱宽度。
    [[nodiscard]] virtual f32 pathfindingWidth() const = 0;

    /// 碰撞箱高度。
    [[nodiscard]] virtual f32 pathfindingHeight() const = 0;

    /// 抬脚高度（原版 `maxUpStep()`）。
    [[nodiscard]] virtual f32 pathfindingStepHeight() const = 0;

    /// 无法安全下落的距离（原版 `getMaxFallDistance()`）。
    [[nodiscard]] virtual i32 pathfindingMaxFallDistance() const = 0;

    /// 该类型对实体的代价；负值表示不可通行（原版 `getPathfindingMalus`）。
    [[nodiscard]] virtual f32 pathfindingMalus(PathType type) const = 0;

    /// 当前碰撞箱（世界坐标）；寻找起点与「能否直线走到」都要用。
    [[nodiscard]] virtual AxisAlignedBB pathfindingBoundingBox() const = 0;

    /// 是否站在地面上（原版 `onGround()`）。
    [[nodiscard]] virtual bool pathfindingOnGround() const = 0;

    /// 是否在水里（原版 `isInWater()`）。
    [[nodiscard]] virtual bool pathfindingInWater() const = 0;

    /// 能否站在这片流体上（原版 `canStandOnFluid`）。
    [[nodiscard]] virtual bool pathfindingCanStandOnFluid(const fluid::FluidState* fluidState) const = 0;
};

/**
 * @brief 一次寻路会话的只读上下文（对应原版 `PathfindingContext`）。
 *
 * 把「世界区域」和「发起寻路的实体位置」绑在一起，避免求值器的每个方法都要带两份参数。
 */
class PathfindingContext {
public:
    PathfindingContext(const Region& region, const PathfindingEntity& entity, const Vector3d& entityPosition)
        : m_region(&region)
        , m_entity(&entity)
        , m_entityPosition(entityPosition)
    {}

    [[nodiscard]] const Region& region() const
    {
        return *m_region;
    }

    [[nodiscard]] const PathfindingEntity& entity() const
    {
        return *m_entity;
    }

    /// 发起寻路的实体位置（原版 `PathfindingContext.mobPosition`，一次会话内保持不变）。
    [[nodiscard]] const Vector3d& entityPosition() const
    {
        return m_entityPosition;
    }

    /// 实体所处的方块坐标。
    [[nodiscard]] BlockPos entityBlockPos() const;

private:
    const Region* m_region;
    const PathfindingEntity* m_entity;
    Vector3d m_entityPosition;
};

/**
 * @brief 寻路求值器基类
 */
class NodeEvaluator {
public:
    NodeEvaluator() = default;
    virtual ~NodeEvaluator() = default;

    NodeEvaluator(const NodeEvaluator&) = delete;
    NodeEvaluator& operator=(const NodeEvaluator&) = delete;

    // ========== 会话 ==========

    /**
     * @brief 开始一次寻路会话
     * @param region 世界区域（会话期间必须保持有效）
     * @param entity 发起寻路的实体
     * @param entityPosition 实体位置
     */
    virtual void prepare(const Region& region, const PathfindingEntity& entity, const Vector3d& entityPosition);

    /**
     * @brief 结束会话并释放节点缓存中的本会话状态
     *
     * 缓存的节点对象保留（`Path` 已按值拷贝，不依赖它们），但求值器与世界、实体的
     * 绑定被解开，此后不得再调用 `getPathType` / `getNeighbors`。
     */
    virtual void done();

    /**
     * @brief 当前会话上下文；不在会话中时为 nullptr
     */
    [[nodiscard]] const PathfindingContext* context() const
    {
        return m_context.get();
    }

    [[nodiscard]] bool inSession() const
    {
        return m_context != nullptr;
    }

    // ========== 子类接口 ==========

    /**
     * @brief 搜索起点节点；无法开始时返回 nullptr
     */
    [[nodiscard]] virtual PathPoint* getStart() = 0;

    /**
     * @brief 搜索目标
     * @param x, y, z 目标坐标（可以是小数，原版对它们取 floor）
     */
    [[nodiscard]] virtual Target getTarget(f64 x, f64 y, f64 z) = 0;

    /**
     * @brief 枚举给定节点的横向 / 跳跃邻居
     * @param neighbors 输出缓冲区，调用方保证至少 `maxNeighbors()` 个位置
     * @param point 当前节点
     * @return 写入的邻居数量
     */
    virtual i32 getNeighbors(PathPoint** neighbors, PathPoint* point) = 0;

    /// 邻居缓冲区需要的最大容量。
    [[nodiscard]] static constexpr i32 maxNeighbors() noexcept
    {
        return 32;
    }

    /**
     * @brief 与实体尺寸无关的单格路径类型
     */
    [[nodiscard]] virtual PathType getPathType(
        const PathfindingContext& context, i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 考虑实体尺寸后的路径类型（原版 `getPathTypeOfMob`）
     */
    [[nodiscard]] virtual PathType getPathTypeOfMob(
        const PathfindingContext& context, i32 x, i32 y, i32 z) const = 0;

    // ========== 能力开关 ==========

    void setCanPassDoors(bool value)
    {
        m_canPassDoors = value;
    }

    void setCanOpenDoors(bool value)
    {
        m_canOpenDoors = value;
    }

    void setCanFloat(bool value)
    {
        m_canFloat = value;
    }

    void setCanWalkOverFences(bool value)
    {
        m_canWalkOverFences = value;
    }

    [[nodiscard]] bool canPassDoors() const
    {
        return m_canPassDoors;
    }

    [[nodiscard]] bool canOpenDoors() const
    {
        return m_canOpenDoors;
    }

    [[nodiscard]] bool canFloat() const
    {
        return m_canFloat;
    }

    [[nodiscard]] bool canWalkOverFences() const
    {
        return m_canWalkOverFences;
    }

    // ========== 节点 ==========

    /**
     * @brief 获取或创建指定坐标的节点
     *
     * 同一会话内同坐标永远返回同一个对象，因此搜索过程中可以用指针判定身份。
     */
    [[nodiscard]] PathPoint* getNode(i32 x, i32 y, i32 z)
    {
        const u64 key = nodeKey(x, y, z);
        const auto found = m_nodes.find(key);
        if (found != m_nodes.end()) {
            return found->second.get();
        }
        auto node = std::make_unique<PathPoint>(x, y, z);
        PathPoint* raw = node.get();
        m_nodes.emplace(key, std::move(node));
        return raw;
    }

    [[nodiscard]] PathPoint* getNode(const BlockPos& pos)
    {
        return getNode(pos.x, pos.y, pos.z);
    }

protected:
    /**
     * @brief 目标节点（原版 `getTargetNodeAt`）：对小数坐标取 floor
     */
    [[nodiscard]] Target _getTargetNodeAt(f64 x, f64 y, f64 z) const;

    /**
     * @brief 节点坐标 → 缓存键
     *
     * 位布局与原版 `Node.createHash` 不同（原版用 15 位打包，依赖「坐标不会超出该范围」），
     * 这里用 64 位完整保留三个坐标，避免远处坐标在哈希里碰撞成同一个节点。
     */
    [[nodiscard]] static u64 nodeKey(i32 x, i32 y, i32 z) noexcept
    {
        return (static_cast<u64>(static_cast<u32>(x)) << 42) ^ (static_cast<u64>(static_cast<u32>(y)) << 21) ^
            static_cast<u64>(static_cast<u32>(z));
    }

    /// 实体宽 / 高 / 深各占几个方块（原版 `prepare` 里的 `floor(尺寸 + 1)`）。
    [[nodiscard]] i32 entityWidthCells() const
    {
        return m_entityWidth;
    }
    [[nodiscard]] i32 entityHeightCells() const
    {
        return m_entityHeight;
    }
    [[nodiscard]] i32 entityDepthCells() const
    {
        return m_entityDepth;
    }

    std::unique_ptr<PathfindingContext> m_context;
    i32 m_entityWidth = 1;
    i32 m_entityHeight = 1;
    i32 m_entityDepth = 1;
    bool m_canPassDoors = true;
    bool m_canOpenDoors = false;
    bool m_canFloat = false;
    bool m_canWalkOverFences = false;

private:
    std::unordered_map<u64, std::unique_ptr<PathPoint>> m_nodes;
};

} // namespace entity::ai::pathfinding
} // namespace mc