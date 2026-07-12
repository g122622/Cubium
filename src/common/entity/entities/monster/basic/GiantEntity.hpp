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

#include "../MonsterEntity.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 巨人实体
 *
 * 非常巨大的敌对生物，只能通过命令生成。
 *
 * 特性：
 * - 巨大体型：高度近12格
 * - 高攻击力：高伤害攻击
 * - 高生命值：100点生命
 * - 无AI：没有智能行为
 * - 无生成：只能通过命令生成
 */
class GiantEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    GiantEntity(EntityId id);
    ~GiantEntity() override = default;

    // 禁止拷贝
    GiantEntity(const GiantEntity&) = delete;
    GiantEntity& operator=(const GiantEntity&) = delete;

    // 允许移动
    GiantEntity(GiantEntity&&) = delete;
    GiantEntity& operator=(GiantEntity&&) = delete;

    /**
     * @brief 创建巨人实体
     * @param world 世界实例
     * @return 新的巨人实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 属性 ==========

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 3.6f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 12.0f; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 10.44f; }

    /**
     * @brief 巨人不会燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 寻路权重 ==========

    /**
     * @brief 获取路径权重
     *
     * 巨人是唯一不取反光照权重的 Monster 子类，
     * 返回 brightness - 0.5f（偏好明亮区域），
     * 与 AnimalEntity 类似但不检查草方块。
     * 对应 MC Giant.getWalkTargetValue。
     */
    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 获取环境音效
     *
     * 巨人无环境音，对齐原版 Giant（不 override getAmbientSound → Mob 默认 null），
     * 避免默认拼接出不存在的 entity.giant.ambient。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;
};

} // namespace mc
