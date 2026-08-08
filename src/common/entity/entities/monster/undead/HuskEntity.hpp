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

#include "../../../../core/Types.hpp"
#include "ZombieEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include <memory>

namespace mc {

/**
 * @brief 尸壳实体
 *
 * 在沙漠中生成的僵尸变种。
 *
 * 特性：
 * - 沙漠生成：在沙漠生物群系生成
 * - 不燃烧：在阳光下不燃烧
 * - 饥饿攻击：攻击会使玩家饥饿
 */
class HuskEntity : public ZombieEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    HuskEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~HuskEntity() override = default;

    // 禁止拷贝
    HuskEntity(const HuskEntity&) = delete;
    HuskEntity& operator=(const HuskEntity&) = delete;

    // 允许移动
    HuskEntity(HuskEntity&&) = delete;
    HuskEntity& operator=(HuskEntity&&) = delete;

    /// 本类继承链标识（parent = ZombieEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    /**
     * @brief 创建尸壳实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 阳光燃烧 ==========

    /**
     * @brief 尸壳不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

protected:
    void registerAttributes() override;
};

} // namespace mc
