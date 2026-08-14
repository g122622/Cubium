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

// 前向声明：customizeArrow 参数仅需引用类型，无需完整定义
namespace entity {
class ArrowEntity;
}

/**
 * @brief 流浪者实体
 *
 * 流浪者是骷髅的冰雪群系变种，主要特征：
 * - 使用弓箭进行远程攻击，射出的箭矢附带 30 秒缓慢 I 效果（对应原版 Arrow of Slowness）
 * - 作为亡灵生物，会在阳光下燃烧（与普通骷髅一致，继承 MonsterEntity 默认 shouldBurnInDaylight=true）
 * - 免疫细雪的冰冻伤害
 * - 生成于雪原/冰刺之地等冰雪生物群系
 *
 * Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_流浪者.txt#行为
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

    // ========== 迟缓之箭定制 ==========
    //
    // 重写 customizeArrow 钩子，在基类 attackEntityWithRangedAttack 创建普通箭矢后、
    // 发射前为箭矢附加缓慢效果，使射出的箭命中目标时施加 30 秒缓慢 I
    // （对应原版流浪者发射 Arrow of Slowness）。
    // ArrowEntity::onEntityHit 会自动将箭矢携带的效果施加给被命中的生物。
    //
    // Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_流浪者.txt#行为（发射造成缓慢效果的箭）
    void customizeArrow(entity::ArrowEntity& arrow) override;

protected:
    void registerAttributes() override;

private:
    /// 流浪者射出箭矢附带的缓慢效果持续时间（ticks），600 ticks = 30 秒（对齐原版）
    static constexpr i32 SLOWNESS_DURATION_TICKS = 600;
    /// 缓慢效果等级（0 = 缓慢 I，对齐原版）
    static constexpr i32 SLOWNESS_AMPLIFIER = 0;
};

} // namespace mc
