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
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/fish/AbstractFishEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <memory>
#include <optional>

namespace mc {

/**
 * @brief 蝌蚪实体
 *
 * 青蛙的幼体，可在水中游动；成年后（24000 tick）转化为青蛙。玩家可用水桶装取
 * 得到蝌蚪桶，或用黏液球（FROG_FOOD 标签）喂养加速成长。
 *
 * 参考: net.minecraft.world.entity.animal.frog.Tadpole
 */
class TadpoleEntity : public AbstractFishEntity {
public:
    /// 成长为青蛙所需的 tick 数（对齐 vanilla Tadpole.ticksToBeFrog = 24000）
    static constexpr i32 TICKS_TO_BE_FROG = 24000;

    TadpoleEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~TadpoleEntity() override = default;

    TadpoleEntity(const TadpoleEntity&) = delete;
    TadpoleEntity& operator=(const TadpoleEntity&) = delete;
    TadpoleEntity(TadpoleEntity&&) = delete;
    TadpoleEntity& operator=(TadpoleEntity&&) = delete;

    static const entity::EntityClassInfo& classInfo();

    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 成长 ==========

    [[nodiscard]] i32 getAge() const noexcept { return m_age; }
    void setAge(i32 age);

    /// 距成年剩余 tick
    [[nodiscard]] i32 getTicksLeftUntilAdult() const noexcept;

    /**
     * @brief 加速成长（喂养黏液球）
     * @param ticks 加速的 tick 数
     */
    void ageUp(i32 ticks);

    // ========== 实体行为 ==========

    void tick() override;

    [[nodiscard]] f32 eyeHeight() const override { return 0.3f * 0.5f; }

    /**
     * @brief 蝌蚪死亡不掉落经验
     *
     * 覆盖 LivingEntity::shouldDropExperience 的 `!isChild()` 语义：蝌蚪永远处于幼体形态，
     * 但按 vanilla Tadpole.shouldDropExperience 恒返回 false，被玩家击杀也不掉经验球。
     */
    [[nodiscard]] bool shouldDropExperience() const override { return false; }

    // ========== 音效 ==========

    [[nodiscard]] std::optional<ResourceLocation> getFlopSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override { return std::nullopt; }
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    // ========== 桶装支持 ==========

    [[nodiscard]] ItemStack getBucketItemStack() const override;

    /// 蝌蚪总是"来自桶"语义（对齐 vanilla Tadpole.fromBucket = true）
    [[nodiscard]] bool isFromBucket() const override { return true; }

    /// 蝌蚪不参与消失（对齐 vanilla 覆写 setFromBucket 为空实现 + fromBucket 恒 true）
    [[nodiscard]] bool preventDespawn() const override { return true; }

protected:
    void registerAttributes() override;

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    /**
     * @brief 变态为青蛙（对齐 vanilla Tadpole.ageUp → convertTo(FROG, ...)）
     *
     * 经 Entity::convertTo 创建青蛙实体并送入世界，随后执行 finalizeSpawn(CONVERSION)、
     * 持久化标记、尺寸修正与 TADPOLE_GROW_UP 音效；蝌蚪本身被静默移除。
     * 仅在服务端生效；无世界或已在移除流程中时不做任何事。
     */
    void _metamorphoseIntoFrog();

    i32 m_age = 0; ///< 年龄（tick），达到 TICKS_TO_BE_FROG 时转化为青蛙
};

} // namespace mc
