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

#include "../monster/MonsterEntity.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <optional>

namespace mc {

// 前向声明
class IWorld;
class DamageSource;
class LivingEntity;
class BlockPos;
class BlockState;

namespace entity {

/**
 * @brief 监守者实体
 *
 * 出自 Minecraft 1.19 "荒野更新"，由 SculkShrieker（幽匿尖啸体）召唤。
 *
 * 特性：
 * - 高生命值 (500)：MC 中血量最高的非 Boss 实体
 * - 高攻击力 (30)：单次攻击可重创钻石套玩家
 * - 击退抗性 (1.0)：完全免疫击退
 * - 攻击击退 (1.5)：击飞目标
 * - 不免疫火焰：可被岩浆点燃（但免疫窒息/溺水/凋零）
 * - 抑制振动：触发 sculk_sensor 时被减弱（dampensVibrations() 返回 true）
 * - 永不自然消失：preventDespawn() 返回 true
 * - 和平难度消失：isDespawnPeaceful() 返回 true
 * - 摔落免疫：onLivingFall() 返回 false
 * - 摸索/Digging/Emerging 阶段免疫所有伤害（除虚空）
 *
 * MC 源码参考：net.minecraft.world.entity.monster.warden.Warden
 *
 * @note 当前实现仅包含基础战斗行为（近战 + 攻击玩家），完整的监守者
 *       行为系统（VibrationSystem 振动感知、AngerManagement 怒气管理、
 *       SonicBoom 音爆攻击、Emerging/Digging 钻地动画、Roar 怒吼、
 *       Sniff 嗅闻、 Darkness 黑暗效果、心跳音效等）暂未实现，
 *       相关位置均留有显式 TODO 注释，便于后续逐项收敛。
 */
class WardenEntity : public MonsterEntity {
public:
    /**
     * @brief 工厂方法
     * @param world 世界实例（当前未使用，但保持与 EntityFactory 签名一致）
     * @return 新的监守者实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    explicit WardenEntity(EntityId id);

    ~WardenEntity() noexcept override = default;

    // 禁止拷贝
    WardenEntity(const WardenEntity&) = delete;
    WardenEntity& operator=(const WardenEntity&) = delete;

    // 禁止移动（基类 CreatureEntity 不可移动）
    WardenEntity(WardenEntity&&) = delete;
    WardenEntity& operator=(WardenEntity&&) = delete;

    // ========== Entity 接口重写 ==========

    /**
     * @brief 实体宽度
     *
     * MC 1.21.11 Warden.getDefaultDimensions() 在非 Digging/Emerging 状态下
     * 宽度为 0.9f。
     */
    [[nodiscard]] f32 width() const override { return 0.9f; }

    /**
     * @brief 实体高度
     *
     * MC 1.21.11 Warden.getDefaultDimensions() 在非 Digging/Emerging 状态下
     * 高度为 2.9f。
     */
    [[nodiscard]] f32 height() const override { return 2.9f; }

    /**
     * @brief 眼睛高度
     *
     * MC 1.21.11 Warden 模型中眼睛高度约为 2.4f（实体高度 2.9f 的 0.83 倍）。
     */
    [[nodiscard]] f32 eyeHeight() const override { return 2.4f; }

    // ========== LivingEntity 接口重写 ==========

    /**
     * @brief 获取环境音效
     *
     * MC 1.21.11 Warden.getAmbientSound() 在非 Roaring/Digging/Emerging 状态下
     * 根据 AngerLevel 返回不同的环境音效。当前实现尚未引入 AngerLevel 概念，
     * 暂统一返回 "ambient" 通用 ID。
     *
     * TODO: 引入 AngerLevel（Angry/Agitated/Calmed）后重写此方法，
     *       根据怒气等级返回不同的环境音效（warden_ambient/warden_agitated/warden_listening）。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     *
     * MC 1.21.11 Warden.getHurtSound() 返回 SoundEvents.WARDEN_HURT。
     * 当前项目 SoundEvents.hpp 未预定义该事件，使用通用 "hurt" ID。
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     *
     * MC 1.21.11 Warden.getDeathSound() 返回 SoundEvents.WARDEN_DEATH。
     * 当前项目 SoundEvents.hpp 未预定义该事件，使用通用 "death" ID。
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 监守者免疫伤害判断
     *
     * MC 1.21.11 Warden.isInvulnerableTo():
     * - 若处于 Digging 或 Emerging 姿态，免疫除"穿透无敌"标签外的所有伤害
     * - 否则交由父类 Monster.isInvulnerableTo() 处理
     *
     * 当前实现尚未引入姿态系统，简化为只免疫溺水/凋零伤害（与基类 MonsterEntity
     * 默认行为一致），姿态相关免疫将在姿态系统实现后接入。
     *
     * TODO: 引入 Pose::DIGGING / Pose::EMERGING 后实现完整的免疫逻辑。
     */
    [[nodiscard]] bool isInvulnerableTo(DamageSource& source) const override;

    /**
     * @brief 是否为非 Boss 实体
     *
     * MC 1.21.11 Warden 虽然不是真正的 Boss（不显示 Boss 条），但具有
     * preventDespawn() == true 的特性，且部分逻辑（如 isNonBoss 控制是否
     * 触发 Boss 级生成限制）返回 false。本项目沿用 WitherEntity 的处理方式，
     * 让监守者返回 isNonBoss() == false，使其与凋灵/末影龙在生成限制上
     * 保持一致。
     */
    [[nodiscard]] bool isNonBoss() const override { return false; }

    /**
     * @brief 摔落伤害免疫
     *
     * MC 1.21.11 Warden 通过 isInvulnerableTo 对 Fall 伤害类型返回 true
     * （继承自 Monster 默认行为外加摔落免疫），实际 Warden 不会受到摔落伤害。
     */
    [[nodiscard]] bool onLivingFall(f32 distance, f32 damageMultiplier) override;

    /**
     * @brief 永不自然消失
     *
     * MC 1.21.11 Warden.removeWhenFarAway() 返回 false，监守者无论距离玩家
     * 多远都不会被自然消失机制清除。
     */
    [[nodiscard]] bool preventDespawn() const override { return true; }

    /**
     * @brief 和平难度下消失
     *
     * 监守者继承自 MonsterEntity，和平难度下会被清除。
     */
    [[nodiscard]] bool isDespawnPeaceful() const override { return true; }

    /**
     * @brief 抑制振动
     *
     * MC 1.21.11 Warden.dampensVibrations() 返回 true，监守者触发的
     * game_event（脚步、跳跃等）会被振动系统减弱。
     */
    [[nodiscard]] bool dampensVibrations() const override { return true; }

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // ========== 常量（参考 MC 1.21.11 Warden） ==========
    static constexpr f32 MAX_HEALTH = 500.0f;         // 监守者最大生命值
    static constexpr f32 MOVEMENT_SPEED = 0.3f;       // 战斗时的移动速度
    static constexpr f32 KNOCKBACK_RESISTANCE = 1.0f; // 完全免疫击退
    static constexpr f32 ATTACK_KNOCKBACK = 1.5f;     // 攻击击退强度
    static constexpr f32 ATTACK_DAMAGE = 30.0f;       // 单次攻击伤害
    static constexpr f32 FOLLOW_RANGE = 24.0f;        // 目标搜索范围
};

} // namespace entity
} // namespace mc
