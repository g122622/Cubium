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

#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"

#include <memory>
#include <optional>
#include <string>

namespace mc {

class IWorld;
class ItemStack;
class DamageSource;
class BlockState;

/**
 * @brief 青蛙实体
 *
 * MC 1.19 引入的两栖动物，由蝌蚪变态而来。本项目无 Brain 系统，原版的
 * FrogAi（长跳、突舌捕食、蛙明灯产卵）尚未移植，此类当前覆盖：
 * - 实体与同步数据（变体 Holder<FrogVariant>、舌击目标 OptionalInt）；
 * - 按生成位置气候选择变体（temperate / warm / cold）；
 * - 属性、音效、坠落下限、水中移动、可食用判定与繁殖产物；
 * - NBT 存读（variant）。
 *
 * 长跳（LONG_JUMPING）与突舌攻击（ShootTongue + TONGUE 姿态）已以
 * FrogLongJumpGoal / FrogTongueGoal 等效实现。
 *
 * TODO(frog-ai): 产卵（IS_PREGNANT → Frogspawn）与黏液球/岩浆怪掉落蛙明灯依赖 Brain 记忆系统，
 *   待 Brain 版青蛙 AI 实现时补齐。
 */
class FrogEntity : public AnimalEntity {
public:
    /// 青蛙变体（对齐 vanilla FrogVariant 注册表 id 顺序：temperate / warm / cold）
    enum class Variant : i32 {
        Temperate = 0, ///< 温带（默认）
        Warm = 1,      ///< 温暖（恶地/沙漠附近的温暖群系）
        Cold = 2,      ///< 寒冷（针叶林/沼泽）
    };

    /// 默认变体（对齐 vanilla Frog.DEFAULT_VARIANT = FrogVariants.TEMPERATE）
    static constexpr Variant DEFAULT_VARIANT = Variant::Temperate;
    /// 坠落伤害减免（对齐 vanilla Frog.FROG_FALL_DAMAGE_REDUCTION = 5）
    static constexpr i32 FROG_FALL_DAMAGE_REDUCTION = 5;
    /// 碰撞箱宽度（对齐 vanilla Frog 默认实体尺寸 0.5）
    static constexpr f32 WIDTH = 0.5f;
    /// 碰撞箱高度（对齐 vanilla Frog 默认实体尺寸 0.5）
    static constexpr f32 HEIGHT = 0.5f;
    /// 眼睛高度（对齐 vanilla Frog 覆盖 getEyeHeight 的 0.5F * 0.45F）
    static constexpr f32 EYE_HEIGHT = 0.5f * 0.45f;

    FrogEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~FrogEntity() override = default;

    FrogEntity(const FrogEntity&) = delete;
    FrogEntity& operator=(const FrogEntity&) = delete;
    FrogEntity(FrogEntity&&) = delete;
    FrogEntity& operator=(FrogEntity&&) = delete;

    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);
    static const entity::EntityClassInfo& classInfo();

    // ========== 变体 ==========

    [[nodiscard]] Variant getVariant() const;
    void setVariant(Variant variant);

    /**
     * @brief 按位置所在生物群系选择变体（对齐 vanilla VariantUtils.selectVariantToSpawn）
     *
     * 群系命中 SPAWNS_WARM_VARIANT_FROGS → Warm；命中 SPAWNS_COLD_VARIANT_FROGS → Cold；
     * 否则 Temperate（标签为空时回退到对应群系的直接判定）。
     *
     * @param world 世界（可为空，为空时返回默认变体）
     * @param pos 判定位置
     * @return 选中的变体
     */
    [[nodiscard]] static Variant selectVariantForPosition(const IWorld* world, const BlockPos& pos);

    // ========== 舌击目标（对齐 vanilla DATA_TONGUE_TARGET_ID） ==========

    /// 设置舌击目标实体（对齐 vanilla Frog.setTongueTarget）
    void setTongueTarget(EntityInstanceId target);
    /// 清除舌击目标（对齐 vanilla Frog.eraseTongueTarget）
    void eraseTongueTarget();
    /// 当前舌击目标 id（无目标时为空，对齐 vanilla getTongueTarget 的 OptionalInt 语义）
    [[nodiscard]] std::optional<EntityInstanceId> tongueTargetId() const;

    // ========== 突舌捕食（对齐 vanilla Frog.canEat / FrogFood 判定） ==========

    /**
     * @brief 青蛙能否捕食该实体（对齐 vanilla Frog.canEat）
     *
     * 对应 MC 1.21.11 Frog.canEat：体型不为 1 的史莱姆不可捕食，
     * 其余实体按 EntityTypeTags.FROG_FOOD（slime / magma_cube）判定。
     */
    [[nodiscard]] static bool canEat(const LivingEntity& entity);

    /**
     * @brief 捕食结算（对齐 vanilla ShootTongue.eatEntity）
     *
     * 播放 FROG_EAT、对舌击目标造成一次近战攻击；目标被击杀后走常规移除流程
     * （掉落物/蛙明灯由实体的 death 链路处理）。
     */
    void eatTongueTarget();

    // ========== 繁殖 ==========

    /**
     * @brief 繁殖物品：FROG_FOOD 标签（黏液球）
     *
     * 对齐 vanilla Frog.isFood：ItemStack.is(ItemTags.FROG_FOOD)。
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体：直接返回青蛙（青蛙无幼体形态）
     *
     * 对齐 vanilla Frog.getBreedOffspring：生成的 AgeableMob 即青蛙本身，且
     * isBaby() 恒为 false 使调用方的 setBaby(true) 不生效。
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 幼体语义 ==========

    /// 青蛙恒为成体（对齐 vanilla Frog.isBaby）
    [[nodiscard]] bool isChild() const override { return false; }
    /// 青蛙不进入幼体状态（对齐 vanilla Frog.setBaby 为空实现）
    void setChild(bool /*child*/) override {}

    // ========== 行为 ==========

    /**
     * @brief 摔落伤害（对齐 vanilla Frog.calculateFallDamage = 基类伤害 − 5）
     *
     * 基类先按通用公式结算（扣到 0 以下则无伤害），随后额外减免 FROG_FALL_DAMAGE_REDUCTION。
     *
     * @param distance 摔落距离
     * @param damageMultiplier 调用方传入的伤害倍率（干草块/蜂蜜块 0.2 等）
     * @param source 摔落伤害来源
     */
    void causeFallDamage(f32 distance, f32 damageMultiplier, const DamageSource& source) override;

    // ========== 尺寸与音效 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return WIDTH; }
    [[nodiscard]] f32 getBaseHeight() const override { return HEIGHT; }
    [[nodiscard]] f32 eyeHeight() const override { return EYE_HEIGHT; }

    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    // ========== 生命周期 ==========

    /**
     * @brief 生成初始化：按生成位置的气候标签选择青蛙变体
     *
     * 对齐 vanilla Frog.finalizeSpawn 的 VariantUtils.selectVariantToSpawn 分支。
     */
    void finalizeSpawn(IWorld& world,
        const entity::combat::DifficultyInstance& difficulty,
        world::spawn::SpawnReason spawnReason) override;

    /**
     * @brief 死亡时将状态复位
     *
     * 清除舌击目标、结束突舌状态机（舌击目标 resetTask 已复位攻击目标与姿态）。
     */
    void die(DamageSource& source) override;

protected:
    void registerGoals() override;
    void registerAttributes() override;
    void registerData() override;
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    /// vanilla EntityDataSerializers 中 FROG_VARIANT 的注册序号（Hub 字段序列化器 id）。
    static constexpr i32 VANILLA_FROG_VARIANT_SERIALIZER_ID = 25;

    /// 对齐 vanilla Frog.DATA_VARIANT_ID（FROG_VARIANT 序列化器）
    static entity::DataParameter<entity::HolderVariantValue> DATA_VARIANT_ID_PARAM;
    /// 对齐 vanilla Frog.DATA_TONGUE_TARGET_ID（OPTIONAL_UNSIGNED_INT 序列化器）
    static entity::DataParameter<entity::OptionalUnsignedIntValue> DATA_TONGUE_TARGET_ID_PARAM;

public:
    /// 变体注册表 id 名（minecraft:temperate / minecraft:warm / minecraft:cold）
    [[nodiscard]] static const char* variantIdName(Variant variant);

private:
    /// 由注册表 id 名解析变体（未知名称返回空）
    [[nodiscard]] static std::optional<Variant> variantFromName(const std::string& name);
};

} // namespace mc