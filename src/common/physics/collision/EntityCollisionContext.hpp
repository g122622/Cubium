/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"

namespace mc {

// 前向声明：EntityCollisionContext 仅持有 Entity 指针，无需完整类型定义，
// 避免 physics 层头文件反向依赖 entity 层（保持 physics 仅依赖 world/block 的单向依赖）。
class Entity;

/**
 * @brief 实体碰撞上下文
 *
 * 对齐 vanilla net.minecraft.world.phys.shapes.EntityCollisionContext / CollisionContext。
 * 携带碰撞形状判定所需的实体相关上下文，供需要按实体区分碰撞形状的方块
 * （如细雪 PowderSnowBlock.getCollisionShape）查询使用。
 *
 * 设计动机：Cubium 的 Block::getCollisionShape(const BlockState&) 签名无实体上下文，
 * 无法表达 vanilla getCollisionShape(state, level, pos, CollisionContext) 中依实体类型
 * /下落距离/是否在方块上方/是否下降中决定的形状。本结构把这些上下文打包，
 * 经物理引擎碰撞收集路径透传到 Block::getCollisionShapeForEntity。
 *
 * 字段语义（对齐 vanilla EntityCollisionContext）：
 * - entity：触发碰撞判定的实体指针（可能为 nullptr，表示无实体上下文，如纯方块放置预检）。
 * - entityBox：实体当前世界坐标 AABB（用于 isAbove 几何判定）。可能为 nullptr。
 * - descending：实体是否正在"下降"——对齐 vanilla CollisionContext.isDescending() =
 *   Entity.isShiftKeyDown()（即玩家潜行态，Cubium 用 Entity::isSneaking()）。用于细雪：穿皮革靴
 *   的玩家按潜行键时主动陷入细雪（细雪返回 empty），非潜行时得完整碰撞箱可行走。注意这与 Y 速度
 *   方向无关：自由下落不算 descending（否则可行走实体下落到细雪顶时会误判 descending 而穿过细雪）。
 *
 * isAbove(shape, blockY) 判定实体是否站在方块顶面上（对齐 vanilla
 * CollisionContext.isAbove(Shapes.block(), pos, false)）：实体 AABB 底部 Y 不低于
 * 方块顶面 Y（blockY + 1）减去容差，即实体脚部位于方块顶部或更高。
 */
struct EntityCollisionContext {
    const Entity* entity = nullptr;
    const AxisAlignedBB* entityBox = nullptr;
    bool descending = false;

    /**
     * @brief 判定实体是否位于方块顶面或更高处
     *
     * 对齐 vanilla CollisionContext.isAbove(Shapes.block(), blockPos, false)：
     * 当实体 AABB 底部 minY >= 方块顶面 (blockY + 1) - EPSILON 时为 true，
     * 即实体站在方块上方（脚部在方块顶部）。用于细雪等"仅当实体在方块上方时才提供
     * 完整碰撞箱使其可行走"的判定。
     *
     * 无实体或无碰撞箱上下文时返回 false（保守视为不在上方，不提供可行走碰撞箱）。
     *
     * @param blockY 方块 Y 坐标
     * @return 实体是否在方块顶面或更高
     */
    [[nodiscard]] bool isAbove(i32 blockY) const noexcept
    {
        if (entityBox == nullptr) {
            return false;
        }
        constexpr f32 EPSILON = 1.0e-5f;
        const f32 blockTopY = static_cast<f32>(blockY) + 1.0f;
        return entityBox->minY >= blockTopY - EPSILON;
    }
};

} // namespace mc
