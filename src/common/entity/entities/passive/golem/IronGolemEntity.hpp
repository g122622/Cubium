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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "GolemEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class VillagerEntity;

/**
 * @brief 铁傀儡实体
 *
 * 保护村民的大型傀儡。
 *
 * 特性：
 * - 保护村民：攻击威胁村民的生物
 * - 中立：对玩家中立，除非被激怒
 * - 举起手臂：攻击时会举起手臂
 * - 击飞：攻击会将敌人向上击飞
 * - 生成：村民足够多时自然生成
 * - 掉落：铁锭、罂粟
 * - 送花：偶尔给村民展示罂粟花
 */
class IronGolemEntity : public GolemEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    IronGolemEntity(EntityInstanceId id);
    ~IronGolemEntity() override = default;

    // 禁止拷贝
    IronGolemEntity(const IronGolemEntity&) = delete;
    IronGolemEntity& operator=(const IronGolemEntity&) = delete;

    // 允许移动
    IronGolemEntity(IronGolemEntity&&) = delete;
    IronGolemEntity& operator=(IronGolemEntity&&) = delete;

    /**
     * @brief 创建铁傀儡实体
     * @param world 世界实例
     * @return 新的铁傀儡实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 攻击状态 ==========

    /**
     * @brief 是否举起手臂
     */
    [[nodiscard]] bool isArmsRaised() const { return m_armsRaised; }

    /**
     * @brief 设置手臂状态
     */
    void setArmsRaised(bool raised) { m_armsRaised = raised; }

    /**
     * @brief 获取攻击计时器
     */
    [[nodiscard]] i32 getAttackTimer() const { return m_attackTimer; }

    /**
     * @brief 设置攻击计时器
     */
    void setAttackTimer(i32 timer) { m_attackTimer = timer; }

    // ========== 生成 ==========

    /**
     * @brief 是否由玩家创建
     */
    [[nodiscard]] bool isPlayerCreated() const { return m_playerCreated; }

    /**
     * @brief 设置玩家创建标记
     */
    void setPlayerCreated(bool created) { m_playerCreated = created; }

    // ========== 送花状态 ==========

    /**
     * @brief 是否手持花朵（给村民展示罂粟花）
     */
    [[nodiscard]] bool isHoldingRose() const { return m_holdRoseTick > 0; }

    /**
     * @brief 获取持花倒计时
     */
    [[nodiscard]] i32 getHoldRoseTick() const { return m_holdRoseTick; }

    /**
     * @brief 设置手持花朵状态
     * @param holding 是否手持花朵
     *
     * 会广播实体状态到客户端，触发动画效果。
     */
    void setHoldingRose(bool holding);

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 2.1f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 1.4f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 2.7f; }

    // ========== 生命周期 ==========

    void tick() override;

    // ========== 战斗 ==========

    /**
     * @brief 近战攻击实体
     *
     * 铁傀儡特有的攻击方式：造成大伤害并将目标向上击飞。
     * 攻击后广播实体状态到客户端，触发攻击动画和音效。
     *
     * @param target 目标实体
     * @return 是否成功攻击
     */
    bool attackEntityAsMob(LivingEntity& target) override;

    /**
     * @brief 播放攻击音效
     *
     * 铁傀儡攻击时播放 ENTITY_IRON_GOLEM_ATTACK 声音。
     */
    void playAttackSound(LivingEntity& target) override;

    /**
     * @brief 检查是否可以攻击指定类型的实体
     *
     * 重写 MobEntity::canAttackType()。
     * 铁傀儡的攻击类型限制：
     * - 玩家创建的铁傀儡不攻击玩家
     * - 铁傀儡永远不攻击苦力怕
     *
     * 对应 Minecraft 原版 IronGolem.canAttackType()。
     *
     * @param typeId 实体类型ID
     * @return 是否可以攻击
     */
    [[nodiscard]] bool canAttackType(const entity::EntityType& type) const override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 获取环境音效
     *
     * 铁傀儡无环境音，对齐原版 AbstractGolem.getAmbientSound 返回 null，
     * 避免默认拼接出不存在的 entity.iron_golem.ambient。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

private:
    // 攻击状态
    bool m_armsRaised = false;
    i32 m_attackTimer = 0;

    // 生成标记
    bool m_playerCreated = false;

    // 送花状态
    i32 m_holdRoseTick = 0;

    // 常量
    static constexpr i32 ATTACK_DURATION = 10;  // 攻击动画持续时间（tick）
    static constexpr f32 ATTACK_DAMAGE = 15.0f; // 攻击伤害基础值（属性注册值），MC 1.21.11 原版为 15.0
};

} // namespace mc
