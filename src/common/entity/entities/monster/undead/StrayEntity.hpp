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

#include "AbstractSkeletonEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 流浪者实体
 *
 * 流浪者是骷髅的变种，主要特征：
 * - 使用弓箭进行远程攻击
 * - 不在阳光下燃烧
 * - 生成于雪原生物群系
 */
class StrayEntity : public AbstractSkeletonEntity {
public:
    StrayEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~StrayEntity() override = default;

    StrayEntity(const StrayEntity&) = delete;
    StrayEntity& operator=(const StrayEntity&) = delete;
    StrayEntity(StrayEntity&&) = delete;
    StrayEntity& operator=(StrayEntity&&) = delete;

    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

protected:
    void registerAttributes() override;
};

} // namespace mc
