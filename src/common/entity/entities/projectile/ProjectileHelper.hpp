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

#include "ProjectileEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntitySize.hpp"

#include <functional>

namespace mc {

// 前向声明
class LivingEntity;
class Item;

namespace entity {

/**
 * @brief 投掷物公共辅助工具
 *
 * 提供投掷物实体的通用功能，包括旋转朝向计算、碰撞检测和实体命中判定。
 */
class ProjectileHelper {
public:
    /**
     * @brief 按运动方向更新实体朝向
     *
     * 根据投掷物的速度向量计算目标朝向，并以指定的旋转速度平滑过渡。
     * 适用于箭矢、雪球等投掷物的飞行姿态更新。
     *
     * @param projectile 投掷物实体
     * @param rotationSpeed 旋转插值系数 (0-1)，值越大旋转越快
     */
    static void rotateTowardsMovement(Entity& projectile, f32 rotationSpeed);

    /**
     * @brief 为当前位移构造实体搜索范围
     *
     * 根据投掷物的位移向量和包围盒，构建一个包含整个运动轨迹的搜索范围。
     * 用于预先筛选可能与投掷物碰撞的实体。
     *
     * @param projectile 投掷物实体
     * @param movement 位移向量
     * @param margin 额外扩展的边距，默认为1.0
     * @return 包含整个运动轨迹的轴对齐包围盒
     */
    [[nodiscard]] static AxisAlignedBB createMovementSearchBox(
        const Entity& projectile, const Vector3& movement, f32 margin = 1.0f);

    /**
     * @brief 在线段上查找最近的实体命中
     *
     * 从起点到终点进行射线检测，找出与线段相交的最近实体。
     * 用于投掷物的实体碰撞检测。
     *
     * @param world 世界接口
     * @param projectile 投掷物实体（排除自身碰撞）
     * @param start 射线起点
     * @param end 射线终点
     * @param searchBox 搜索范围
     * @param filter 实体过滤器，返回true表示该实体可被命中
     * @param collisionExpansion 碰撞箱扩展半径，默认为0.3
     * @return 射线检测结果，包含命中类型和位置信息
     */
    [[nodiscard]] static RayTraceResult rayTraceEntities(const IWorld& world,
        const Entity& projectile,
        const Vector3& start,
        const Vector3& end,
        const AxisAlignedBB& searchBox,
        const std::function<bool(const Entity&)>& filter,
        f32 collisionExpansion = 0.3f);
};

} // namespace entity

/**
 * @brief 获取实体持有指定武器物品的手
 *
 * 先检查主手是否持有指定物品，如果是则返回主手，否则返回副手。
 * 对应 MC 原版 ProjectileUtil.getWeaponHoldingHand()。
 *
 * @param entity 实体
 * @param item 要查找的物品
 * @return 持有该物品的手（主手或副手）
 */
Hand getWeaponHoldingHand(const LivingEntity& entity, const Item* item);

} // namespace mc
