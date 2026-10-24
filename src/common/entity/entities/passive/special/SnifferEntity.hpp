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
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

// 前向声明
class IWorld;
class ItemStack;
class DamageSource;

/**
 * @brief 嗅探兽实体
 *
 * MC 1.21.11 引入的古代被动生物，通过嗅探兽蛋孵化获得。
 *
 * 特性：
 * - 挖掘系统：成体可在特定方块上挖掘，掉落火把花种子/瓶草荚果
 * - 状态机：Idling / FeelingHappy / Scenting / Sniffing / Searching / Digging / Rising
 * - 繁殖：用火把花种子/瓶草荚果繁殖，繁殖后掉落嗅探兽蛋物品（非直接幼体）
 * - 幼体：幼年期 48000 tick（40 分钟），是普通动物的两倍
 *
 * 参考：net.minecraft.world.entity.animal.sniffer.Sniffer
 *
 * @note 原版使用 Brain + MemoryModuleType（SNIFFER_SNIFFING_TARGET / SNIFFER_DIGGING /
 *       SNIFFER_HAPPY / SNIFF_COOLDOWN / SNIFFER_EXPLORED_POSITIONS）驱动状态机；本项目无 Brain
 *       系统，改以 GoalSelector 上的 SnifferSniffGoal（嗅探）→ SnifferSearchGoal（搜索）→
 *       SnifferDigGoal（挖掘）→ SnifferHappyGoal（开心）链式衔接，并以实体字段承载上述记忆语义
 *       （hasSniffingTarget / isHappy / isSniffCoolingDown / 已探索位置列表）。
 */
namespace entity::ai::goal {
class PanicGoal;
class TemptGoal;
} // namespace entity::ai::goal

class SnifferEntity : public AnimalEntity {
public:
    /**
     * @brief 嗅探兽状态枚举
     *
     * 对齐 MC 1.21.11 Sniffer.State，id 与原版一致以保证网络同步兼容。
     */
    enum class State : i8 {
        Idling = 0,       ///< 空闲
        FeelingHappy = 1, ///< 感到开心（繁殖后）
        Scenting = 2,     ///< 闻气味
        Sniffing = 3,     ///< 嗅探
        Searching = 4,    ///< 搜索挖掘点
        Digging = 5,      ///< 挖掘中
        Rising = 6,       ///< 抬头（挖掘结束）
    };

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    SnifferEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~SnifferEntity() override = default;

    // 禁止拷贝
    SnifferEntity(const SnifferEntity&) = delete;
    SnifferEntity& operator=(const SnifferEntity&) = delete;

    // 允许移动
    SnifferEntity(SnifferEntity&&) = delete;
    SnifferEntity& operator=(SnifferEntity&&) = delete;

    /**
     * @brief 实体工厂方法
     *
     * 用于 EntityRegistry 注册
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 状态机 ==========

    /**
     * @brief 获取当前状态
     *
     * wire 上 DATA_STATE_PARAM 以 SnifferStateValue(VarInt(State.id), serializerId=31 SNIFFER_STATE)
     * 存储以对齐 vanilla 1.21.11 Sniffer.DATA_STATE；此处包装回 State 枚举。
     */
    [[nodiscard]] State getState() const
    {
        return static_cast<State>(m_dataManager.get<entity::SnifferStateValue>(DATA_STATE_PARAM).stateId);
    }

    /**
     * @brief 设置当前状态
     *
     * 对齐 MC Sniffer.setState：仅更新同步数据，不触发声音/动画。
     * 若需播放对应声音（如 SNIFFER_HAPPY、SNIFFER_SNIFFING），应使用 transitionTo()。
     */
    void setState(State state)
    {
        m_dataManager.set(DATA_STATE_PARAM, entity::SnifferStateValue{static_cast<i32>(state)});
    }

    /**
     * @brief 状态转换（带声音效果）
     *
     * 对齐 MC Sniffer.transitionTo：根据目标状态播放对应声音。
     * - FeelingHappy: SNIFFER_HAPPY
     * - Scenting: SNIFFER_SCENTING（幼体音调 1.3）
     * - Sniffing: SNIFFER_SNIFFING
     * - Rising: SNIFFER_DIGGING_STOP
     *
     * @note Digging 状态额外执行 onDiggingStart：设置 DROP_SEED_AT_TICK 时刻、广播
     *       实体状态 63（客户端据此播放挖掘循环音效），并刷新碰撞箱/眼高。
     *
     * @param state 目标状态
     */
    void transitionTo(State state);

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     *
     * 对齐 MC Sniffer.isFood：使用 ItemTags.SNIFFER_FOOD 标签（数据包键 minecraft:sniffer_food）。
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    // ========== 繁殖产出 ==========

    /**
     * @brief 繁殖产出：在本体位置掉落一枚嗅探兽蛋（不生成幼体）
     *
     * 对齐 MC 1.21.11 Sniffer.spawnChildFromBreeding（Sniffer.java:337-344）：繁殖产物是
     * SNIFFER_EGG 物品实体而非幼体；物品按原版默认动量在本体位置生成，拾取延迟为默认值，
     * 并播放 SNIFFER_EGG_PLOP 音效（音高 (nextFloat - nextFloat) * 0.2 + 0.5）。
     *
     * @param partner 交配伙伴（原版此处未使用）
     * @return 恒为 nullptr（本次繁殖不产生幼体实体，`bred_animals` 进度因此不会命中 child 条件）
     * @note 本轮繁殖的发情重置、繁殖冷却、爱心粒子与经验球由 BreedGoal::spawnBaby() 统一处理。
     */
    AnimalEntity* spawnChildFromBreeding(AnimalEntity& partner) override;

    /**
     * @brief 生成幼体（BreedGoal 的纯生成契约）
     *
     * 嗅探兽的繁殖产出被 spawnChildFromBreeding 覆盖为掉蛋，本方法不参与生产繁殖链路，
     * 仅保留契约以满足 AnimalEntity 的纯虚接口；返回值恒为 nullptr。
     *
     * @param partner 交配伙伴
     * @return 恒为 nullptr
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 嗅探状态机（对齐 vanilla Sniffer 的 can* 判定与记忆模块） ==========

    /// 是否可以嗅探（对齐 vanilla Sniffer.canSniff）
    [[nodiscard]] bool canSniff() const;

    /// 是否可以挖掘（对齐 vanilla Sniffer.canDig 的实体侧前置条件，方块侧由 SnifferDigGoal 判定）
    [[nodiscard]] bool canDig() const;

    /**
     * @brief 是否已有嗅探目标（对齐 vanilla MemoryModuleType.SNIFFER_SNIFFING_TARGET 存在）
     *
     * 目标存在时嗅探兽不进入挖掘，而是先寻路到该目标。
     */
    [[nodiscard]] bool hasSniffingTarget() const noexcept
    {
        return m_hasSniffingTarget;
    }

    void setSniffingTarget(bool value) noexcept
    {
        m_hasSniffingTarget = value;
    }

    /// 标记下次嗅探结束时掉种（对齐 vanilla onDiggingComplete(timedOut) 的效果）
    void setShouldDropSeed(bool value) noexcept
    {
        m_shouldDropSeed = value;
    }

    /// 取出并清除“本次嗅探结束时掉种”标记
    [[nodiscard]] bool consumeShouldDropSeed();

    /// 开心标记（对齐 vanilla MemoryModuleType.SNIFFER_HAPPY）
    [[nodiscard]] bool isHappy() const noexcept
    {
        return m_happy;
    }

    void setHappy(bool value) noexcept
    {
        m_happy = value;
    }

    /// 设置嗅探冷却 tick（对齐 vanilla SNIFF_COOLDOWN，挖掘超时后 9600 tick）
    void setSniffCooldownTicks(u64 ticks);
    /// 是否处于嗅探冷却中
    [[nodiscard]] bool isSniffCoolingDown() const noexcept;

    /// 该位置是否已挖掘过（对齐 vanilla SNIFFER_EXPLORED_POSITIONS 记忆）
    [[nodiscard]] bool isExploredPosition(const BlockPos& pos) const;
    /// 记录已挖掘位置（对齐 vanilla Sniffer.storeExploredPosition，最多 20 个）
    void storeExploredPosition(const BlockPos& pos);

    /// 食物诱惑目标（对齐 vanilla TEMPTING_PLAYER / IS_TEMPTED 记忆）
    [[nodiscard]] entity::ai::goal::TemptGoal* temptGoal() const noexcept
    {
        return m_temptGoal;
    }

    /// 恐慌逃跑目标（对齐 vanilla IS_PANICKING 记忆）
    [[nodiscard]] entity::ai::goal::PanicGoal* panicGoal() const noexcept
    {
        return m_panicGoal;
    }

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     *
     * 对齐 MC Sniffer.getAmbientSound：Digging / Searching 状态不播放环境音。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    // ========== 尺寸 ==========

    /**
     * @brief 获取眼睛高度
     *
     * 对齐 MC Sniffer 默认 1.05F（基类 AnimalEntity 的眼睛高度）。
     * 挖掘状态下改用 DIGGING_DIMENSIONS 的 0.81F 眼高（见 getDimensions）。
     */
    [[nodiscard]] f32 eyeHeight() const override
    {
        // 默认成体 1.05 / 幼体 0.525（对齐 vanilla Animal.getDefaultDimensions 的眼高）；
        // 挖掘状态下由 getDimensions 提供 DIGGING_DIMENSIONS 的 0.81。
        if (getState() == State::Digging) {
            return isChild() ? DIGGING_EYE_HEIGHT * babyScale() : DIGGING_EYE_HEIGHT;
        }
        return isChild() ? 0.525f : 1.05f;
    }

    [[nodiscard]] f32 getBaseWidth() const override
    {
        return 1.9f;
    }

    [[nodiscard]] f32 getBaseHeight() const override
    {
        return 1.75f;
    }

    /**
     * @brief 实体尺寸
     *
     * 对齐 MC Sniffer.getDefaultDimensions：挖掘状态使用 DIGGING_DIMENSIONS
     * （高度 −0.4、眼高 0.81），并按幼体缩放。状态切换时由 transitionTo 刷新。
     */
    [[nodiscard]] entity::EntitySize getDimensions(EntityPose pose) const override;

    /**
     * @brief 是否可播放挖掘循环音效（对齐 vanilla Sniffer.canPlayDiggingSound）
     *
     * 挖掘与搜索状态为真；客户端收到实体状态 63 后据此决定是否维持音效。
     */
    [[nodiscard]] bool canPlayDiggingSound() const
    {
        const State state = getState();
        return state == State::Digging || state == State::Searching;
    }

    // ========== 幼体设置 ==========

    /**
     * @brief 设置幼体状态
     *
     * 对齐 MC Sniffer.setBaby：嗅探兽幼年期为 48000 tick（40 分钟），
     * 是普通动物（24000 tick / 20 分钟）的两倍，因此需覆盖 AgeableEntity::setChild
     * 来设置正确的幼年期长度。
     *
     * @param baby 是否为幼体
     */
    void setChild(bool baby) override;

    /**
     * @brief 嗅探兽幼年期 tick 数（对齐 MC Sniffer.SNIFFER_BABY_AGE_TICKS = 48000，40 分钟）
     *
     * 嗅探兽幼年期是普通动物（AgeableEntity::BABY_AGE = -24000）的两倍，
     * SnifferEggBlock::randomTick 孵化幼体时需通过 setChild(true) 触发本类覆盖，
     * 以设置 -48000 的年龄值。
     */
    static constexpr i32 SNIFFER_BABY_AGE_TICKS = 48000;

    /// 挖掘开始到掉出种子的延迟 tick（对齐 vanilla Sniffer.DIGGING_DROP_SEED_OFFSET_TICKS）
    static constexpr i32 DIGGING_DROP_SEED_OFFSET_TICKS = 120;
    /// 挖掘开始时向客户端广播的实体状态（对齐 vanilla broadcastEntityEvent(this, (byte)63)）
    static constexpr u8 DIGGING_ENTITY_STATUS = 63;
    /// 挖掘状态高度减少量 / 眼高（对齐 vanilla Sniffer.DIGGING_DIMENSIONS）
    static constexpr f32 DIGGING_HEIGHT_REDUCTION = 0.4f;
    static constexpr f32 DIGGING_EYE_HEIGHT = 0.81f;
    /// 挖掘粒子开始延迟 / 结束时刻 / 每 tick 数量（对齐 vanilla Sniffer 同名常量）
    static constexpr i32 DIGGING_PARTICLES_DELAY_TICKS = 1700;
    static constexpr i32 DIGGING_PARTICLES_DURATION_TICKS = 6000;
    static constexpr i32 DIGGING_PARTICLES_AMOUNT = 30;

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 距离掉种的剩余 tick（DATA_DROP_SEED_AT_TICK − 当前 tick；未安排掉种时无意义）
     *
     * 供测试与调试观察挖掘时序，避免暴露同步参数本身。
     */
    [[nodiscard]] i32 ticksUntilDropSeed() const;

    /**
     * @brief 挖掘中在头部方块位置喷出方块粒子（对齐 vanilla Sniffer.emitDiggingParticles）
     *
     * 仅在挖掘开始后 DIGGING_PARTICLES_DELAY_TICKS ~ DIGGING_PARTICLES_DURATION_TICKS 区间内生效，
     * 每 tick 喷出 DIGGING_PARTICLES_AMOUNT 个方块粒子，每 10 tick 播放一次方块击打音效。
     *
     * @param diggingTicks 进入挖掘状态后经过的 tick 数
     */
    void emitDiggingParticles(i32 diggingTicks);

    /**
     * @brief 挖掘到点后从嗅探兽战利品表掉出物品（对齐 vanilla Sniffer.dropSeed）
     *
     * 仅在服务端、当前状态为 Digging 且 DATA_DROP_SEED_AT_TICK 等于当前 tick 时生效；
     * 掉落物生成在头部方块位置，并播放 SNIFFER_DROP_SEED 音效。
     */
    void dropSeed();

    /**
     * @brief 死亡时重置状态机
     *
     * 对齐 MC Java 1.21.11 Sniffer.die（Sniffer.java:347-350）：
     *   public void die(DamageSource p_277689_) {
     *       this.transitionTo(Sniffer.State.IDLING);
     *       super.die(p_277689_);
     *   }
     * 嗅探兽死亡时将状态重置为 Idling（vanilla 通过 transitionTo 播放状态切换音效，
     * Idling 分支 transitionTo 仅 setState 不播音，与 super.die 前置调用语义一致），
     * 再委托 AnimalEntity::die 执行通用死亡逻辑。
     */
    void die(DamageSource& source) override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 数据同步 ==========
    void registerData() override;

    // ========== 脚步声 ==========
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;

    // ========== NBT 序列化 ==========
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    /**
     * @brief 进入挖掘状态时的初始化（对齐 vanilla Sniffer.onDiggingStart）
     *
     * 记录 DROP_SEED_AT_TICK = tickCount + 120，并向追踪玩家广播实体状态 63
     * （客户端据此播放 SNIFFER_DIGGING 循环音效）。
     */
    void _onDiggingStart();

    // ========== 数据同步参数 ==========
    /// 对齐 MC Sniffer.DATA_STATE（SNIFFER_STATE 序列化器 id=31, wire=VarInt(State.id)）
    /// @note vanilla 1.21.11 SNIFFER_STATE 是独立 serializer(VarInt),非 Byte。旧实现误用 i8(BYTE)
    ///       致真客户端类型校验崩,改为 SnifferStateValue 对齐。getState/setState 做 State↔id 互转。
    static entity::DataParameter<entity::SnifferStateValue> DATA_STATE_PARAM;
    /// 对齐 MC Sniffer.DATA_DROP_SEED_AT_TICK（INT 序列化器）
    /// @note 挖掘链路写入"掉落种子时刻"，由 dropSeed 在该 tick 兑现。
    static entity::DataParameter<i32> DATA_DROP_SEED_AT_TICK_PARAM;

protected:
    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

private:
    /// vanilla Sniffer：已探索位置记忆最多保留 20 个
    static constexpr std::size_t MAX_EXPLORED_POSITIONS = 20;

    /// 嗅探目标是否已确定（对齐 vanilla MemoryModuleType.SNIFFER_SNIFFING_TARGET 存在）
    bool m_hasSniffingTarget = false;
    /// 下次嗅探结束时是否掉种（对齐 vanilla Sniffer.onDiggingComplete(timedOut) 的效果）
    bool m_shouldDropSeed = false;
    /// 本次调用 dropSeed 是否绕开 DATA_DROP_SEED_AT_TICK 时序门控（嗅探后直接掉种）
    bool m_dropSeedPending = false;
    /// 开心标记（对齐 vanilla MemoryModuleType.SNIFFER_HAPPY）
    bool m_happy = false;
    /// 嗅探冷却截止时刻（对齐 vanilla MemoryModuleType.SNIFF_COOLDOWN）
    u64 m_sniffCooldownUntil = 0;
    /// 已探索位置（世界坐标，最新在前，最多 MAX_EXPLORED_POSITIONS 个）
    std::vector<BlockPos> m_exploredPositions;
    /// AI 目标指针（查询执行中状态用，生命周期随 m_goalSelector）
    entity::ai::goal::TemptGoal* m_temptGoal = nullptr;
    entity::ai::goal::PanicGoal* m_panicGoal = nullptr;
};

} // namespace mc
