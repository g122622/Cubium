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
#include "../../../core/DataParameter.hpp"
#include "../../../core/EntityDataManager.hpp"
#include "../basic/AnimalEntity.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include <memory>
#include <optional>

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
 * @note 当前实现为最小可玩版本：完整的属性/声音/NBT/数据同步/繁殖/孵化集成，
 *       但复杂的 Brain AI 状态机（嗅探→搜索→挖掘→掉落种子）暂未实现，
 *       后续可在此基础上扩展。原版使用 Brain + MemoryModuleType.SNIFFER_EXPLORED_POSITIONS
 *       管理挖掘状态，本项目使用 GoalSelector，需要单独设计对应的 Goal。
 */
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
     * @note Digging 状态的原版逻辑（设置 DROP_SEED_AT_TICK、广播实体状态）暂未实现，
     *       详见 onDiggingStart() 的 TODO 注释。
     *
     * @param state 目标状态
     */
    void transitionTo(State state);

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     *
     * 对齐 MC Sniffer.isFood：使用 ItemTags.SNIFFER_FOOD 标签。
     * 当前项目无物品标签系统，直接判断 TORCHFLOWER_SEEDS 或 PITCHER_POD。
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    /**
     * @brief 生成幼体
     *
     * @note MC 原版 Sniffer.spawnChildFromBreeding 实际掉落 SNIFFER_EGG 物品而非直接生成幼体。
     *       当前项目 Items::SNIFFER_EGG 尚未实现，且 BreedGoal 契约要求返回幼体，
     *       因此本方法返回幼体嗅探兽作为占位实现。
     *       TODO: 待 Items::SNIFFER_EGG 实现后，改为覆盖 spawnChildFromBreeding 掉落蛋物品。
     *
     * @param partner 交配伙伴
     * @return 生成的幼体实体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

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
     * @note 原版 DIGGING_DIMENSIONS 使用 0.81F 眼高，当前未实现挖掘尺寸切换。
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.525f : 1.05f; }

    [[nodiscard]] f32 getBaseWidth() const override { return 1.9f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 1.75f; }

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

    // ========== 生命周期 ==========

    void tick() override;

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
    // ========== 数据同步参数 ==========
    /// 对齐 MC Sniffer.DATA_STATE（SNIFFER_STATE 序列化器 id=31, wire=VarInt(State.id)）
    /// @note vanilla 1.21.11 SNIFFER_STATE 是独立 serializer(VarInt),非 Byte。旧实现误用 i8(BYTE)
    ///       致真客户端类型校验崩,改为 SnifferStateValue 对齐。getState/setState 做 State↔id 互转。
    static entity::DataParameter<entity::SnifferStateValue> DATA_STATE_PARAM;
    /// 对齐 MC Sniffer.DATA_DROP_SEED_AT_TICK（INT 序列化器）
    /// @note 用于挖掘完成后掉落种子的时序控制，当前未使用，保留以兼容原版 NBT。
    static entity::DataParameter<i32> DATA_DROP_SEED_AT_TICK_PARAM;

protected:
    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

private:
    /// 状态机最小持续时间（用于 Searching 状态播放音效，对齐 MC Sniffer.tick 中的 playSearchingSound）
    /// @note 当前仅用于 tick() 中 Searching 状态的客户端音效触发，未实际使用。
};

} // namespace mc
