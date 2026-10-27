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
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"

#include <memory>
#include <optional>

namespace mc {

// 前向声明
class Player;
class DamageSource;
class ItemStack;

namespace entity::ai::goal {
class PanicGoal;
class GoatRamGoal;
} // namespace entity::ai::goal

/**
 * @brief 山羊实体（对齐 vanilla 1.21.11 Goat）
 *
 * 山地被动动物：
 * - 2% 概率生成"尖叫山羊"（耳朵与音效不同，冲撞冷却更短）；
 * - 10% 概率生成缺一只角（只影响外观与掉落）；
 * - 冲撞：蓄力 20 tick 后以 3.0 倍速撞向 4~7 格内的目标，命中造成攻击伤害 +
 *   2.5 倍击退（幼体 1.0），撞上 `#snaps_goat_horn` 方块掉落山羊角；
 * - 摔落伤害减免 10（vanilla GOAT_FALL_DAMAGE_REDUCTION）；
 * - 可用桶挤奶（幼年不可）。
 *
 * 与 vanilla 的差异（Brain → GoalSelector 迁移）：vanilla 的 GoatAi 由 RAM / LONG_JUMP /
 * IDLE 三个活动驱动；Cubium 用 GoatRamGoal 以计时器实现 RAM 活动（蓄力→冲撞→冷却），
 * 长跳（LONG_JUMP 活动）由 GoatLongJumpGoal 实现；长跳中姿态为 LongJumping，
 * 碰撞箱缩放为 0.7 倍（vanilla LONG_JUMPING_DIMENSIONS）。
 */
class GoatEntity : public AnimalEntity {
public:
    /// 尖叫山羊生成概率（vanilla GOAT_SCREAMING_CHANCE）
    static constexpr f64 GOAT_SCREAMING_CHANCE = 0.02;
    /// 单角（缺一只角）生成概率（vanilla UNIHORN_CHANCE）
    static constexpr f64 UNIHORN_CHANCE = 0.1;
    /// 摔落伤害减免（vanilla GOAT_FALL_DAMAGE_REDUCTION）
    static constexpr i32 GOAT_FALL_DAMAGE_REDUCTION = 10;
    /// 蓄力冲撞所需 tick（vanilla GoatAi.RAM_PREPARE_TIME）
    static constexpr i32 RAM_PREPARE_TIME = 20;
    /// 冲撞目标最大距离（vanilla GoatAi.RAM_MAX_DISTANCE）
    static constexpr i32 RAM_MAX_DISTANCE = 7;
    /// 冲撞目标最小距离（vanilla GoatAi.RAM_MIN_DISTANCE）
    static constexpr i32 RAM_MIN_DISTANCE = 4;
    /// 冲撞速度倍率（vanilla SPEED_MULTIPLIER_WHEN_RAMMING）
    static constexpr f64 RAM_SPEED_MULTIPLIER = 3.0;
    /// 蓄力走近速度倍率（vanilla SPEED_MULTIPLIER_WHEN_PREPARING_TO_RAM）
    static constexpr f64 RAM_PREPARE_SPEED = 1.25;
    /// 成年/幼年冲撞击退力度（vanilla ADULT/BABY_RAM_KNOCKBACK_FORCE）
    static constexpr f32 ADULT_RAM_KNOCKBACK_FORCE = 2.5f;
    static constexpr f32 BABY_RAM_KNOCKBACK_FORCE = 1.0f;
    /// 普通/尖叫山羊冲撞冷却区间（vanilla TIME_BETWEEN_RAMS / TIME_BETWEEN_RAMS_SCREAMER）
    static constexpr i32 RAM_COOLDOWN_MIN = 600;
    static constexpr i32 RAM_COOLDOWN_MAX = 6000;
    static constexpr i32 RAM_COOLDOWN_SCREAMER_MIN = 100;
    static constexpr i32 RAM_COOLDOWN_SCREAMER_MAX = 300;

    /// DATA_IS_SCREAMING_GOAT / DATA_HAS_LEFT_HORN / DATA_HAS_RIGHT_HORN 的 wire serializerId
    /// （均为 vanilla EntityDataSerializers.BOOLEAN == 8）
    static constexpr i32 VANILLA_BOOLEAN_SERIALIZER_ID = 8;

    GoatEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~GoatEntity() noexcept override = default;

    GoatEntity(const GoatEntity&) = delete;
    GoatEntity& operator=(const GoatEntity&) = delete;
    GoatEntity(GoatEntity&&) = delete;
    GoatEntity& operator=(GoatEntity&&) = delete;

    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

    void registerData() override;

    // ========== 尖叫山羊 / 角 ==========

    [[nodiscard]] bool isScreamingGoat() const;
    void setScreamingGoat(bool screaming);
    [[nodiscard]] bool hasLeftHorn() const;
    [[nodiscard]] bool hasRightHorn() const;
    void addHorns();
    void removeHorns();

    /**
     * @brief 掉落一只山羊角（对齐 vanilla Goat.dropHorn）
     *
     * 优先掉落仍存在的那只角；两只都在时随机选一只。掉落后生成山羊角物品实体。
     * @return 有角可掉且成功生成物品返回 true
     */
    bool dropHorn();

    /**
     * @brief 生成该山羊对应的山羊角物品（对齐 vanilla Goat.createHorn）
     *
     * vanilla 从 SCREAMING_GOAT_HORNS / REGULAR_GOAT_HORNS 乐器标签按 UUID 哈希取乐器；
     * Cubium 乐器注册表尚未接入，主手类型为普通山羊角。
     * TODO(instrument-registry): 乐器注册表落地后按上表标签选取，并把 Instrument 组件写入 ItemStack。
     */
    [[nodiscard]] ItemStack createHorn() const;

    // ========== 冲撞状态机（GoatRamGoal 使用）==========

    /// 冲撞冷却剩余 tick（对齐 vanilla RAM_COOLDOWN_TICKS 记忆）
    [[nodiscard]] i32 ramCooldown() const
    {
        return m_ramCooldown;
    }

    /// 长跳中的姿态缩放（vanilla Goat.LONG_JUMPING_DIMENSIONS = 0.9x1.3 scale 0.7）
    static constexpr f32 LONG_JUMPING_SCALE = 0.7f;

    /// 按 UUID 哈希从乐器标签挑选山羊角音色（对齐 vanilla Goat.createHorn）
    [[nodiscard]] ResourceLocation pickHornInstrument() const;

    /// 长跳中收缩碰撞箱（对齐 vanilla Goat.getDefaultDimensions 的 Pose.LONG_JUMPING 分支）
    [[nodiscard]] entity::EntitySize getDimensions(EntityPose pose) const override;
    void setRamCooldown(i32 ticks);
    /// 采样一次冲撞冷却（尖叫山羊 100~300，普通 600~6000）
    [[nodiscard]] i32 pickNextRamCooldown() const;

    /// 是否正在低头（对齐 vanilla isLoweringHead，由实体事件 58/59 切换）
    [[nodiscard]] bool isLoweringHead() const
    {
        return m_isLoweringHead;
    }

    void setLoweringHead(bool lowering);

    /// 低头进度 0..20（对齐 vanilla lowerHeadTick）
    [[nodiscard]] i32 lowerHeadTick() const
    {
        return m_lowerHeadTick;
    }

    // ========== 交互 / 伤害 / 声音 ==========

    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    [[nodiscard]] i32 fallDamageReduction() const override
    {
        return GOAT_FALL_DAMAGE_REDUCTION;
    }

    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getStepSound() const;

    // ========== 繁殖 / 食物 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    void finalizeSpawn(IWorld& world,
        const entity::combat::DifficultyInstance& difficulty,
        world::spawn::SpawnReason spawnReason) override;

    // ========== 生命周期 ==========

    void tick() override;
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    // ========== 同步数据参数 ==========

    /// DATA_IS_SCREAMING_GOAT（Boolean，默认 false）
    static entity::DataParameter<bool> DATA_IS_SCREAMING_GOAT_PARAM;
    /// DATA_HAS_LEFT_HORN（Boolean，默认 true）
    static entity::DataParameter<bool> DATA_HAS_LEFT_HORN_PARAM;
    /// DATA_HAS_RIGHT_HORN（Boolean，默认 true）
    static entity::DataParameter<bool> DATA_HAS_RIGHT_HORN_PARAM;

    [[nodiscard]] static u16 getScreamingParamId()
    {
        return DATA_IS_SCREAMING_GOAT_PARAM.id();
    }

    [[nodiscard]] static u16 getHasLeftHornParamId()
    {
        return DATA_HAS_LEFT_HORN_PARAM.id();
    }

    [[nodiscard]] static u16 getHasRightHornParamId()
    {
        return DATA_HAS_RIGHT_HORN_PARAM.id();
    }

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    /// Java String.hashCode（山羊角音色种子，对齐 vanilla RandomSource.create(uuid.hashCode())）
    static i32 _javaStringHashCode(const std::string& text);

    /// 吃草音效（尖叫山羊用 screaming_eat）
    void _playEatingSound();

    /// 播放 entity.goat.<suffix> 音效（音效标识缺失时静默跳过）
    void _playSound(const char* suffix, f32 pitch);

    i32 m_ramCooldown = 0;         // 冲撞冷却倒计时
    bool m_isLoweringHead = false; // 低头（蓄力冲撞）
    i32 m_lowerHeadTick = 0;       // 低头动画进度 0..20
};

} // namespace mc
