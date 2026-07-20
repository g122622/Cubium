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
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 可投掷实体基类
 *
 * 用于雪球、鸡蛋、末影珍珠等可投掷物品。
 * 提供重力、碰撞检测和基本的投掷物理。
 */
class ThrowableEntity : public ProjectileEntity {
public:
    ~ThrowableEntity() override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    // ========== ThrowableEntity 方法 ==========

    /**
     * @brief 获取重力加速度
     *
     * 可投掷物品有固定的重力加速度 0.03
     */
    [[nodiscard]] f32 getGravity() const override { return 0.03f; }

protected:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    explicit ThrowableEntity(EntityInstanceId id);
};

} // namespace entity
} // namespace mc
