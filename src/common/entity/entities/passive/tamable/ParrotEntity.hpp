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

#include "ShoulderRidingEntity.hpp"

#include "../../../interfaces/IFlyingAnimal.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"

#include <memory>

namespace mc {

/**
 * @brief 鹦鹉实体
 *
 * 肩膀停驻语义由 `ShoulderRidingEntity` 承载。
 */
class ParrotEntity : public ShoulderRidingEntity, public entity::IFlyingAnimal {
public:
    /**
     * @brief 鹦鹉变种
     */
    enum class ParrotVariant : u8 { RedBlue = 0, Blue = 1, Green = 2, YellowBlue = 3, Gray = 4 };

    /**
     * @brief 构造鹦鹉实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    ParrotEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~ParrotEntity() override = default;

    ParrotEntity(const ParrotEntity&) = delete;
    ParrotEntity& operator=(const ParrotEntity&) = delete;
    ParrotEntity(ParrotEntity&&) noexcept = delete;
    ParrotEntity& operator=(ParrotEntity&&) noexcept = delete;

    /**
     * @brief 创建鹦鹉实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 获取鹦鹉变种
     */
    [[nodiscard]] ParrotVariant getVariant() const { return m_variant; }

    /**
     * @brief 设置鹦鹉变种
     */
    void setVariant(ParrotVariant variant) { m_variant = variant; }

    /**
     * @brief 随机设置变种
     */
    void randomizeVariant();

    /**
     * @brief 当前是否飞行
     */
    [[nodiscard]] bool isFlying() const override { return m_flying; }

    /**
     * @brief 设置飞行状态
     */
    void setFlying(bool flying) override { m_flying = flying; }

    /**
     * @brief 鹦鹉始终可以飞
     */
    [[nodiscard]] bool canFly() const { return true; }

    /**
     * @brief 当前是否正在模仿声音
     */
    [[nodiscard]] bool isImitating() const { return m_imitating; }

    /**
     * @brief 设置模仿状态
     */
    void setImitating(bool imitating) { m_imitating = imitating; }

    /**
     * @brief 获取模仿目标类型 ID
     */
    [[nodiscard]] u32 getImitatingTarget() const { return m_imitatingTarget; }

    /**
     * @brief 设置模仿目标
     */
    void setImitatingTarget(u32 entityType)
    {
        m_imitatingTarget = entityType;
        m_imitating = true;
    }

    /**
     * @brief 鹦鹉使用种子驯服
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const override;

    /**
     * @brief 鹦鹉不能繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override
    {
        (void)itemStack;
        return false;
    }

    /**
     * @brief 鹦鹉不能生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override
    {
        (void)partner;
        return nullptr;
    }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.25f; }

    void tick() override;

    /**
     * @brief 处理玩家交互
     *
     * 用种子驯服鹦鹉（1/10 概率），已驯服的鹦鹉可以切换坐下状态。
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

protected:
    void registerGoals() override;
    void registerAttributes() override;
    [[nodiscard]] f32 getBaseWidth() const override { return 0.5f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.9f; }
    void onTamed(bool tamed) override;

private:
    ParrotVariant m_variant = ParrotVariant::RedBlue;
    bool m_flying = false;
    bool m_imitating = false;
    u32 m_imitatingTarget = 0;
    i32 m_imitateTimer = 0;
    i32 m_flapTimer = 0;
    f32 m_flapSpeed = 0.0f;

    static constexpr i32 IMITATE_INTERVAL_MIN = 100;
    static constexpr i32 IMITATE_INTERVAL_MAX = 600;
    static constexpr f32 FLAP_SPEED_GROUND = 0.0f;
    static constexpr f32 FLAP_SPEED_FLYING = 0.4f;
};

} // namespace mc
