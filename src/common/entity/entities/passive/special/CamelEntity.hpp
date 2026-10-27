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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"

#include <memory>
#include <optional>

namespace mc {

namespace entity::ai::goal {
class PanicGoal;
} // namespace entity::ai::goal

/**
 * @brief 骆驼实体（对齐 vanilla 1.21.11 Camel，继承 AbstractHorse）
 *
 * 沙漠坐骑，可同时载两名乘客：
 * - 默认已驯服（vanilla Camel.isTamed() 恒 true），Shift+右键打开坐骑背包；
 * - 冲刺：骑乘中按跳跃键触发，水平 22.2222 * MOVEMENT_SPEED、垂直 1.4285 * jumpPower，
 *   冷却 55 tick；冲刺中（冷却 < 50）落地/入水/乘载具即结束；
 * - 坐下/站起：空闲 20 秒后可随机坐下，受伤/被牵拉/被骑乘前进/入水时立刻站起；
 *   坐姿高度比站立低 1.43 格、眼高 0.845；
 * - 骑乘时玩家冲刺且冲刺冷却就绪可加速 0.1（vanilla RUNNING_SPEED_BONUS）。
 *
 * 与 vanilla 的差异（Brain → GoalSelector 迁移）：vanilla 由 CamelAi 的 IDLE 活动驱动，
 * Cubium 以等价优先级注册 swim/panic/繁殖/诱惑/跟随父母/漫步/注视目标；随机坐下由
 * CamelSitGoal 按 vanilla RandomSitting 条件与 20 秒最小间隔实现。
 */
class CamelEntity : public AbstractHorseEntity {
public:
    /// 幼体缩放（vanilla Camel.BABY_SCALE）
    static constexpr f32 BABY_SCALE = 0.45f;
    /// 冲刺冷却（vanilla DASH_COOLDOWN_TICKS）
    static constexpr i32 DASH_COOLDOWN_TICKS = 55;
    /// 冲刺结束的冷却阈值（vanilla tick 中 dashCooldown < 50 时结束）
    static constexpr i32 DASH_END_COOLDOWN_THRESHOLD = 50;
    /// 冲刺水平/垂直动量系数（vanilla DASH_HORIZONTAL_MOMENTUM / DASH_VERTICAL_MOMENTUM）
    static constexpr f64 DASH_HORIZONTAL_MOMENTUM = 22.2222;
    static constexpr f64 DASH_VERTICAL_MOMENTUM = 1.4285;
    /// 骑乘冲刺加成（vanilla RUNNING_SPEED_BONUS）
    static constexpr f32 RUNNING_SPEED_BONUS = 0.1f;
    /// 坐下/站起动画时长（vanilla SITDOWN_DURATION_TICKS / STANDUP_DURATION_TICKS）
    static constexpr i64 SITDOWN_DURATION_TICKS = 40;
    static constexpr i64 STANDUP_DURATION_TICKS = 52;
    /// 坐姿与站立的高度差（vanilla SITTING_HEIGHT_DIFFERENCE）
    static constexpr f32 SITTING_HEIGHT_DIFFERENCE = 1.43f;
    /// 随机坐下前的最小空闲时长（vanilla IDLE_MINIMAL_DURATION_TICKS = 80 tick）
    static constexpr i64 IDLE_MINIMAL_DURATION_TICKS = 80;
    /// 坐姿眼高（vanilla SITTING_DIMENSIONS.withEyeHeight(0.845F)）
    static constexpr f32 SITTING_EYE_HEIGHT = 0.845f;

    CamelEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~CamelEntity() noexcept override = default;

    CamelEntity(const CamelEntity&) = delete;
    CamelEntity& operator=(const CamelEntity&) = delete;
    CamelEntity(CamelEntity&&) = delete;
    CamelEntity& operator=(CamelEntity&&) = delete;

    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /// 本类继承链标识（parent = AbstractHorseEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

    void registerData() override;

    // ========== 坐姿 ==========

    /// 是否坐下（对齐 vanilla isCamelSitting：LAST_POSE_CHANGE_TICK < 0）
    [[nodiscard]] bool isCamelSitting() const;
    /// 姿态变化后经过的 tick（对齐 vanilla getPoseTime）
    [[nodiscard]] i64 getPoseTime() const;
    /// 是否处于坐下/站起动画过渡中（对齐 vanilla isInPoseTransition）
    [[nodiscard]] bool isInPoseTransition() const;

    /// 坐下/过渡中拒绝移动（对齐 vanilla refuseToMove）
    [[nodiscard]] bool refuseToMove() const
    {
        return isCamelSitting() || isInPoseTransition();
    }

    /// 是否有控制乘客（对齐 vanilla hasControllingPassenger，用于随机坐下前置条件）
    [[nodiscard]] bool hasControllingPassenger() const;

    /// 是否正在恐慌（PanicGoal 运行中；等价 vanilla CamelAi 的 IS_PANICKING 记忆）
    [[nodiscard]] bool isPanicking() const;

    /// 坐下（对齐 vanilla sitDown）
    void sitDown();
    /// 站起（对齐 vanilla standUp）
    void standUp();
    /// 立刻站起并跳过过渡动画（对齐 vanilla standUpInstantly）
    void standUpInstantly();

    /// 姿态时间戳（对齐 vanilla LAST_POSE_CHANGE_TICK；负值表示坐着）
    [[nodiscard]] i64 lastPoseChangeTick() const;
    void resetLastPoseChangeTick(i64 tick);

    // ========== 冲刺 ==========

    [[nodiscard]] bool isDashing() const;
    void setDashing(bool dashing);

    [[nodiscard]] i32 dashCooldown() const
    {
        return m_dashCooldown;
    }

    /// 骑乘中触发冲刺（对齐 vanilla executeRidersJump）
    void executeDash(f32 jumpScale, Player& rider);

    [[nodiscard]] f32 getAgeScaleValue() const
    {
        return isChild() ? BABY_SCALE : 1.0f;
    }

    // ========== 骑乘 / 交互 ==========

    /// 骆驼恒为已驯服（对齐 vanilla Camel.isTamed）
    [[nodiscard]] bool isTame() const
    {
        return true;
    }

    /// 骑乘速度：MOVEMENT_SPEED +（玩家冲刺且冷却就绪时的 0.1 加成）
    [[nodiscard]] f32 getRiddenSpeed(Player& rider) const;

    [[nodiscard]] bool canBeSprinted() const
    {
        return true;
    }

    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    /// 受伤时立刻站起（对齐 vanilla actuallyHurt）
    void actuallyHurt(DamageSource& source, f32 amount) override;

    // ========== 声音 ==========

    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;
    std::optional<ResourceLocation> getEatSound() const override;

    // ========== 繁殖 / 食物 ==========

    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const override;
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    void finalizeSpawn(IWorld& world,
        const entity::combat::DifficultyInstance& difficulty,
        world::spawn::SpawnReason spawnReason) override;

    // ========== 生命周期 ==========

    void tick() override;

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    // ========== 同步数据参数 ==========

    /// DASH（Boolean，默认 false）
    static entity::DataParameter<bool> DASH_PARAM;
    /// LAST_POSE_CHANGE_TICK（Long，默认 0；负值表示坐姿）
    static entity::DataParameter<i64> LAST_POSE_CHANGE_TICK_PARAM;

    [[nodiscard]] static u16 getDashParamId()
    {
        return DASH_PARAM.id();
    }

    [[nodiscard]] static u16 getLastPoseTickParamId()
    {
        return LAST_POSE_CHANGE_TICK_PARAM.id();
    }

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    /// 播放 entity.camel.<suffix> 音效（音效标识缺失时静默跳过）
    void _playSound(const char* suffix, f32 pitch = 1.0f);

    i32 m_dashCooldown = 0;
    entity::ai::goal::PanicGoal* m_panicGoal = nullptr; // 恐慌目标指针（不拥有）
};

} // namespace mc
