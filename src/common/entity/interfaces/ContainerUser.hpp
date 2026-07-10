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

#include "common/core/Types.hpp"

namespace mc {

// 前向声明
class BlockPos;
class LivingEntity;

namespace entity {

/**
 * @brief 容器使用者接口
 *
 * 实现 this 接口的实体能够"打开"容器方块（如箱子），并通过此接口向
 * 容器系统声明自身当前持有哪个 BlockPos 的容器。容器系统（如 ChestEntity）
 * 在 `_recheckOpeners` 中通过此接口查询附近实体是否在交互范围内，
 * 以便将非玩家打开者（如铜傀儡）计入打开计数并触发箱子动画。
 *
 * 对应 MC 1.21.11: net.minecraft.world.entity.ContainerUser
 *   boolean hasContainerOpen(BlockPos pos);
 *   double getContainerInteractionRange();
 *   default LivingEntity getLivingEntity() { return (LivingEntity) this; }
 */
class ContainerUser {
public:
    virtual ~ContainerUser() = default;

    /**
     * @brief 检查当前是否打开了指定位置的容器
     *
     * 实现应持有「openedContainerPos」字段：当实体通过 TransportItemsBetweenContainers
     * 等 Goal 与容器交互时，调用 setOpenedContainerPos 设置；交互结束时清除。
     *
     * 双箱合并场景下，若实体打开了双箱的某一半，对另一半位置也应返回 true。
     *
     * @param pos 待检查的容器位置
     * @return 如果实体当前打开此容器（或其双箱另一半）返回 true
     */
    [[nodiscard]] virtual bool hasContainerOpen(const BlockPos& pos) const = 0;

    /**
     * @brief 获取容器交互范围（半径，单位：方块）
     *
     * 用于容器在 `_recheckOpeners` 中筛选附近实体：仅当实体到容器中心的距离
     * 小于等于此值时才视为"在交互"。
     *
     * MC 1.21.11 中 CopperGolem 返回 3.0。
     *
     * @return 交互半径
     */
    [[nodiscard]] virtual f64 getContainerInteractionRange() const = 0;

    /**
     * @brief 获取实现此接口的 LivingEntity 指针
     *
     * 用于让容器系统将此实体作为 LivingEntity 处理（如距离计算、gameEvent 来源）。
     * 默认实现通过 static_cast 转换，要求实现类同时继承 LivingEntity。
     */
    [[nodiscard]] virtual LivingEntity* getLivingEntity() = 0;
    [[nodiscard]] virtual const LivingEntity* getLivingEntity() const = 0;
};

} // namespace entity
} // namespace mc
