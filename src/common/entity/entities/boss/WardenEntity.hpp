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

#include "../../core/DataParameter.hpp"
#include "../../core/EntityPose.hpp"
#include "../monster/MonsterEntity.hpp"
#include "WardenAngerLevel.hpp"
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
class Entity;

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
 * - 怒气等级（WardenAngerLevel）：根据 m_anger 切换 Calmed/Agitated/Angry，
 *   影响环境音效（getAmbientSound）和客户端同步（CLIENT_ANGER_LEVEL）
 *
 * MC 源码参考：net.minecraft.world.entity.monster.warden.Warden
 *
 * @note 当前实现包含基础战斗行为（近战 + 攻击玩家）与简化版怒气系统
 *       （单一聚合 m_anger + WardenAngerLevel 枚举）。完整的监守者行为系统
 *       （VibrationSystem 振动感知、AngerManagement 按目标怒气管理、
 *       SonicBoom 音爆攻击、Emerging/Digging 钻地动画、Roar 怒吼、
 *       Sniff 嗅闻、Darkness 黑暗效果、心跳音效等）暂未实现，
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
    explicit WardenEntity(EntityInstanceId id);

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
     * MC 1.21.11 Warden.getAmbientSound():
     * @code
     * return !this.hasPose(Pose.ROARING) && !this.isDiggingOrEmerging()
     *     ? this.getAngerLevel().getAmbientSound()
     *     : null;
     * @endcode
     *
     * 当前项目尚未引入 Pose::ROARING/DIGGING/EMERGING，因此姿态判断
     * 暂时省略（默认 always 返回 anger-based sound）。一旦 Pose 系统扩展，
     * 需在此处补充姿态判断逻辑。
     *
     * 怒气等级对应音效（与 MC 一致）：
     * - Calmed   → SoundEvents::ENTITY_WARDEN_AMBIENT
     * - Agitated → SoundEvents::ENTITY_WARDEN_AGITATED
     * - Angry    → SoundEvents::ENTITY_WARDEN_ANGRY
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     *
     * MC 1.21.11 Warden.getHurtSound() 返回 SoundEvents.WARDEN_HURT。
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     *
     * MC 1.21.11 Warden.getDeathSound() 返回 SoundEvents.WARDEN_DEATH。
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

    // ========== 怒气等级（AngerLevel） ==========

    /**
     * @brief 获取当前怒气等级
     *
     * 对应 MC 1.21.11 Warden.getAngerLevel() = AngerLevel.byAnger(getActiveAnger())。
     * 怒气等级由 m_anger 经 wardenAngerLevelByAnger() 反查得到：
     * - anger < 40   → Calmed
     * - 40 ≤ anger < 80 → Agitated
     * - anger ≥ 80   → Angry
     *
     * @note 当前实现为简化版：MC 原版怒气来自 AngerManagement.getActiveAnger(target)
     *       （按当前目标查询），项目尚未引入 AngerManagement，使用单一聚合
     *       怒气值 m_anger 代替。完整实现 AngerManagement 后应替换为按目标查询。
     */
    [[nodiscard]] WardenAngerLevel getAngerLevel() const noexcept;

    /**
     * @brief 获取客户端同步的怒气值
     *
     * 对应 MC 1.21.11 Warden.getClientAngerLevel()，返回通过
     * EntityDataManager 同步到客户端的怒气值（i32）。客户端用此值
     * 推断怒气等级以调整心跳频率、触须动画等表现。
     *
     * @return 同步怒气值（≥ 0）
     */
    [[nodiscard]] i32 getClientAngerLevel() const noexcept;

    /**
     * @brief 增加怒气值
     *
     * 对应 MC 1.21.11 Warden.increaseAngerAt(Entity, int, boolean) 的简化版。
     * 服务端调用：受到伤害、被触碰、感知到振动时累加怒气。
     * 怒气值上限为 ANGER_LIMIT（150），超过后保持不变。
     *
     * @param amount 增加的怒气值（≥ 0）
     * @return 累加后的怒气值（用于调用方判断是否触发"首次愤怒"等行为）
     *
     * @note MC 原版在增加怒气后会播放 listeningSound；项目当前未实现
     *       listeningSound 触发逻辑，留待 VibrationSystem 接入后补充。
     */
    i32 increaseAnger(i32 amount) noexcept;

    /**
     * @brief 清空怒气值
     *
     * 对应 MC 1.21.11 Warden.clearAnger(Entity)。当目标死亡或离开
     * 感知范围时调用。项目当前为单一聚合怒气，清空后 m_anger 归零。
     */
    void clearAnger() noexcept;

protected:
    // ========== 数据参数注册 ==========
    void registerData() override;

    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== AI 任务更新（每 tick 调用） ==========
    /**
     * @brief 服务端 AI 步进钩子
     *
     * 重写以执行监守者特有的周期性逻辑：
     * - 每 ANGERMANAGEMENT_TICK_DELAY (20) tick 衰减怒气
     *   （对应 MC 1.21.11 Warden.customServerAiStep 中的 angerManagement.tick）
     * - 同步客户端怒气值（对应 syncClientAngerLevel）
     *
     * @note MC 原版在 customServerAiStep 中调用 angerManagement.tick 进行
     *       每实体怒气衰减/转移，项目当前为简化版：直接对 m_anger 减去
     *       ANGER_DECAY_PER_TICK * ANGERMANAGEMENT_TICK_DELAY。
     */
    void updateAITasks() override;

private:
    // ========== 数据参数 ==========
    /// 客户端同步怒气值（对应 MC CLIENT_ANGER_LEVEL）
    static DataParameter<i32> CLIENT_ANGER_LEVEL;

protected:
    /// 本类继承链标识（parent = MonsterEntity::classInfo()）。见 Entity::classInfo()。
    static const EntityClassInfo& classInfo();

private:
    // ========== 常量（参考 MC 1.21.11 Warden） ==========
    static constexpr f32 MAX_HEALTH = 500.0f;         // 监守者最大生命值
    static constexpr f32 MOVEMENT_SPEED = 0.3f;       // 战斗时的移动速度
    static constexpr f32 KNOCKBACK_RESISTANCE = 1.0f; // 完全免疫击退
    static constexpr f32 ATTACK_KNOCKBACK = 1.5f;     // 攻击击退强度
    static constexpr f32 ATTACK_DAMAGE = 30.0f;       // 单次攻击伤害
    static constexpr f32 FOLLOW_RANGE = 24.0f;        // 目标搜索范围

    // ========== 怒气相关常量（参考 MC 1.21.11 Warden / AngerLevel） ==========
    /// 怒气管理 tick 间隔（对应 MC ANGERMANAGEMENT_TICK_DELAY = 20）
    static constexpr i32 ANGERMANAGEMENT_TICK_DELAY = 20;
    /// 怒气上限（防止怒气无限增长，MC 中 AngerManagement 内部同样有上限）
    static constexpr i32 ANGER_LIMIT = 150;
    /// 每个 ANGERMANAGEMENT_TICK_DELAY 衰减的怒气量（对应 MC AngerManagement.tick 的衰减率）
    static constexpr i32 ANGER_DECAY_PER_TICK_INTERVAL = 1;

    // ========== 怒气状态（服务端权威） ==========
    /// 当前聚合怒气值（≥ 0）。简化版，未按目标分别记录。
    /// 完整实现 AngerManagement 后将替换为按实体跟踪的怒气表。
    i32 m_anger = 0;
};

} // namespace entity
} // namespace mc
