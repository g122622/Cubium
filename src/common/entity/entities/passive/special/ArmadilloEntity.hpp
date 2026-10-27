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
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace mc {

// 前向声明
class Player;
class DamageSource;
class ItemStack;

namespace entity::ai::goal {
class PanicGoal;
} // namespace entity::ai::goal

/**
 * @brief 犰狳状态（对齐 vanilla Armadillo.ArmadilloState，wire id 即 ordinal）
 *
 * - Idle(0)：常态，可交互、可繁殖
 * - Rolling(1)：刚受威胁，卷成球（10 tick 动画）
 * - Scared(2)：保持卷球并偶尔探头（50 tick 动画）
 * - Unrolling(3)：危险解除后展开（30 tick 动画）
 *
 * `shouldHideInShell(inStateTicks)` 决定碰撞箱是否收缩为球（vanilla 同名方法）。
 */
enum class ArmadilloState : i32 {
    Idle = 0,
    Rolling = 1,
    Scared = 2,
    Unrolling = 3,
};

/**
 * @brief 犰狳实体（对齐 vanilla 1.21.11 Armadillo）
 *
 * 恶地/热带草原的被动动物：
 * - 受威胁时卷成球，受伤减半（(damage-1)/2），危险持续则在球内等待；
 * - 每 5~10 分钟自然掉落犰狳鳞甲；用刷子可刷下鳞甲并消耗 16 点耐久；
 * - 幼年个体不刷鳞甲、不繁殖；
 * - 受到环境伤害（PANIC_ENVIRONMENTAL_CAUSES）时展开并逃跑。
 *
 * 与 vanilla 的差异（Brain → GoalSelector 迁移）：vanilla 的
 * ArmadilloAi.ArmadilloBallUp 由 PANIC 活动驱动，记忆 DANGER_DETECTED_RECENTLY
 * 决定卷球/探头/展开时序；Cubium 用 ArmadilloBallUpGoal 以计时器等价实现同一时序。
 */
class ArmadilloEntity : public AnimalEntity {
public:
    /// 幼年缩放（vanilla Armadillo.BABY_SCALE）
    static constexpr f32 BABY_SCALE = 0.6f;
    /// 球状态下头部水平转动上限（vanilla MAX_HEAD_ROTATION_EXTENT 取整）
    static constexpr f32 MAX_HEAD_ROTATION_EXTENT = 32.5f;
    /// 受威胁检测间隔（vanilla SCARE_CHECK_INTERVAL）
    static constexpr i32 SCARE_CHECK_INTERVAL = 80;
    /// 水平惊吓距离（vanilla SCARE_DISTANCE_HORIZONTAL）
    static constexpr f64 SCARE_DISTANCE_HORIZONTAL = 7.0;
    /// 垂直惊吓距离（vanilla SCARE_DISTANCE_VERTICAL）
    static constexpr f64 SCARE_DISTANCE_VERTICAL = 2.0;
    /// 危险记忆持续时间（vanilla Armadillo.actuallyHurt 的 DANGER_DETECTED_RECENTLY 80 tick）
    static constexpr i32 DANGER_MEMORY_DURATION = 80;
    /// 球形态最长持续时间（vanilla ArmadilloBallUp.BALL_UP_STAY_IN_STATE = 5 分钟）
    static constexpr i32 BALL_UP_STAY_IN_STATE = 5 * 60 * 20;
    /// 危险剩余时间高于此阈值视为"危险仍在身边"（vanilla DANGER_DETECTED_RECENTLY_DANGER_THRESHOLD）
    static constexpr i32 DANGER_RECENTLY_THRESHOLD = 75;
    /// 探头前/后的判定延迟（vanilla TICKS_DELAY_TO_DETERMINE_IF_DANGER_IS_STILL_AROUND）
    static constexpr i32 DETERMINE_DANGER_DELAY = 5;
    /// 刷犰狳消耗的耐久（vanilla Armadillo.mobInteract 的 hurtAndBreak(16)）
    static constexpr i32 BRUSH_DURABILITY_COST = 16;

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    ArmadilloEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~ArmadilloEntity() noexcept override = default;

    ArmadilloEntity(const ArmadilloEntity&) = delete;
    ArmadilloEntity& operator=(const ArmadilloEntity&) = delete;
    ArmadilloEntity(ArmadilloEntity&&) = delete;
    ArmadilloEntity& operator=(ArmadilloEntity&&) = delete;

    /**
     * @brief 实体工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

    /// DATA_ID_STATE 的 wire serializerId（vanilla EntityDataSerializers.ARMADILLO_STATE == 32）
    static constexpr i32 VANILLA_ARMADILLO_STATE_SERIALIZER_ID = 32;

    /**
     * @brief 注册同步数据参数（DATA_ID_STATE）
     *
     * C++ 虚函数在基类构造中不派生到子类，构造函数须显式调用。
     */
    void registerData() override;

    // ========== 状态 ==========

    [[nodiscard]] ArmadilloState getState() const;
    void switchToState(ArmadilloState state);

    /// 是否处于非 IDLE 状态（对齐 vanilla isScared）
    [[nodiscard]] bool isScared() const
    {
        return getState() != ArmadilloState::Idle;
    }

    /// 是否藏在壳里（对齐 vanilla shouldHideInShell）
    [[nodiscard]] bool shouldHideInShell() const;

    /// ROLLING 动画结束应转入 SCARED（对齐 vanilla shouldSwitchToScaredState）
    [[nodiscard]] bool shouldSwitchToScaredState() const;

    /**
     * @brief 是否应能保持卷球
     *
     * 对齐 vanilla canStayRolledUp：不在恐慌、不在液体、未被拴绳、无乘客、非载具。
     */
    [[nodiscard]] bool canStayRolledUp() const;

    /**
     * @brief 该实体会否惊吓到本犰狳（对齐 vanilla isScaredBy）
     *
     * 在 (7, 2, 7) 膨胀框内，且满足以下任一：亡灵实体 / 最近攻击者 / 冲刺中或骑乘中的玩家。
     */
    [[nodiscard]] bool isScaredBy(const LivingEntity& other) const;

    /// 进入当前状态的 tick 数（对齐 vanilla inStateTicks）
    [[nodiscard]] i64 inStateTicks() const
    {
        return m_inStateTicks;
    }

    /// 危险记忆剩余 tick（等价 vanilla Brain 的 DANGER_DETECTED_RECENTLY 记忆过期时间）
    [[nodiscard]] i32 dangerTicksRemaining() const
    {
        return m_dangerTicks;
    }

    /// 最近一次"危险在身边"判定的结果（对齐 vanilla dangerWasAround）
    [[nodiscard]] bool dangerWasAround() const
    {
        return m_dangerWasAround;
    }

    /// 下一次探头计时
    [[nodiscard]] i32 nextPeekTimer() const
    {
        return m_nextPeekTimer;
    }

    /// 最近一次惊吓检测的 tick（对齐 vanilla SCARE_CHECK_INTERVAL 节流）
    [[nodiscard]] i64 lastScareCheckTick() const
    {
        return m_lastScareCheckTick;
    }

    /// 是否正在恐慌逃跑（PanicGoal 运行中；等价 vanilla ArmadilloAi 的 IS_PANICKING 记忆）
    [[nodiscard]] bool isPanicking() const;

    // ========== 动作 ==========

    /// 卷成球（对齐 vanilla rollUp）
    void rollUp();

    /// 展开（对齐 vanilla rollOut）
    void rollOut();

    /**
     * @brief 刷掉鳞甲（对齐 vanilla brushOffScute）
     *
     * 幼年返回 false（不刷）；否则掉落 1 个犰狳鳞甲、播放刷子音效并返回 true。
     */
    bool brushOffScute(Player* player);

    // ========== 鳞甲自然掉落 ==========

    /// 距离下次自然掉落鳞甲的剩余 tick（对齐 vanilla scuteTime）
    [[nodiscard]] i32 scuteTime() const
    {
        return m_scuteTime;
    }

    void setScuteTime(i32 ticks)
    {
        m_scuteTime = ticks;
    }

    /// 刷新下次自然掉落计时（20 分钟起，随机追加 0~20 分钟）
    void resetScuteTime();

    // ========== 交互 / 伤害 ==========

    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    bool hurt(DamageSource& source, f32 amount) override;

    /// 受伤回调（记录危险记忆并按需卷球/展开，对齐 vanilla actuallyHurt）
    void actuallyHurt(DamageSource& source, f32 amount) override;

    /// 眼睛高度：球形态时眼睛降到靠近中心的 0.4（对齐 vanilla Armadillo 卷球时的视线点）
    [[nodiscard]] f32 eyeHeight() const override;

    /// 幼体缩放（vanilla Armadillo.getAgeScale；项目无 getAgeScale 虚函数，供渲染/尺寸查询使用）
    [[nodiscard]] f32 ageScale() const
    {
        return isChild() ? BABY_SCALE : 1.0f;
    }

    /// 移动控制标记：卷球时禁止移动/跳跃（对齐 vanilla 的 MOVE_TO_TARGET 门控）
    void updateMovementGoalFlags() override;

    // ========== 声音 ==========

    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;
    /// 脚步声（项目无 getStepSound 虚函数，由 playStepSound 直接取用）
    [[nodiscard]] std::optional<ResourceLocation> getStepSound() const;

    // ========== 繁殖 / 食物 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 生命周期 ==========

    void tick() override;

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    // ========== 测试/诊断 ==========

    [[nodiscard]] static u16 getStateParamId()
    {
        return DATA_ID_STATE_PARAM.id();
    }

    /// 同步数据参数：DATA_ID_STATE（wire 序列化器 ARMADILLO_STATE=32，值 VarInt(状态 id)）
    static entity::DataParameter<i32> DATA_ID_STATE_PARAM;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    /// 危险记忆衰减与"危险仍在身边"判定（对齐 ArmadilloBallUp.tick 前半段）
    void _tickDangerMemory();

    /// 有攻击者实体造成伤害时的危险处理（卷球）
    void _onHurtByEntity(const DamageSource& source, bool isEnvironmentalPanicCause);

    /// 播放 entity.armadillo.<suffix> 音效（音效资源缺失时静默跳过）
    void _playSoundOrNothing(std::string_view suffix);

    i64 m_inStateTicks = 0;                             // 当前状态持续 tick（对应 vanilla inStateTicks）
    i32 m_scuteTime = 0;                                // 下次自然掉落鳞甲的倒计时
    i32 m_dangerTicks = 0;                              // 危险记忆剩余 tick（DANGER_DETECTED_RECENTLY 等价）
    i32 m_nextPeekTimer = 0;                            // 下次探头倒计时（SCARED 状态）
    bool m_dangerWasAround = false;                     // 危险判定是否发生过切换（用于重置探头计时）
    i64 m_lastScareCheckTick = -1;                      // 上次惊吓检测 tick（SCARE_CHECK_INTERVAL 节流）
    entity::ai::goal::PanicGoal* m_panicGoal = nullptr; // 恐慌目标指针（isPanicking 查询，不拥有）
};

} // namespace mc
