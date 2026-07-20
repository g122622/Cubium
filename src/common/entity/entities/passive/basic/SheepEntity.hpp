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
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../util/color/DyeColor.hpp"
#include "AnimalEntity.hpp"
#include "common/entity/ai/goal/goals/EatGrassGoal.hpp"
#include "common/entity/interfaces/IShearable.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace mc {

// 前向声明
class ItemStack;
class Player;
class DamageSource;
class Block;

/**
 * @brief 羊实体
 *
 * 可剪羊毛的被动动物，用小麦繁殖。
 * 实现 IShearable 接口以支持剪羊毛功能。
 */
class SheepEntity : public AnimalEntity, public entity::IShearable {
public:
    SheepEntity(EntityInstanceId id);
    ~SheepEntity() override = default;

    /**
     * @brief 实体工厂方法
     *
     * 用于 EntityRegistry 注册
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
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

    // ========== 羊毛颜色 ==========

    /**
     * @brief 获取羊毛颜色
     * @return 羊毛颜色
     */
    [[nodiscard]] DyeColor getFleeceColor() const { return m_fleeceColor; }

    /**
     * @brief 设置羊毛颜色
     */
    void setFleeceColor(DyeColor color) { m_fleeceColor = color; }

    /**
     * @brief 是否被剪过（没有羊毛）
     */
    [[nodiscard]] bool isSheared() const { return m_sheared; }

    /**
     * @brief 设置剪毛状态
     */
    void setSheared(bool sheared) { m_sheared = sheared; }

    // ========== IShearable 接口实现 ==========

    /**
     * @brief 是否可以被剪毛
     */
    [[nodiscard]] bool isShearable() const override;

    /**
     * @brief 剪毛
     */
    std::vector<ItemStack> shear(Player* player = nullptr) override;

    [[nodiscard]] i32 getShearCooldown() const override { return m_shearCooldown; }

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 吃草 ==========

    /**
     * @brief 吃草奖励
     *
     * 当羊吃草时调用：
     * - 如果被剪过，重新长出羊毛
     * - 如果是幼羊，加速成长60 ticks
     */
    void eatGrassBonus();

    // ========== 颜色混合 ==========

    /**
     * @brief 从父母颜色获取混合后的幼羊颜色
     *
     * @param parent1Color 父母1的颜色
     * @param parent2Color 父母2的颜色
     * @param random 随机数生成器
     * @return 混合后的颜色（如果没有配方则随机选择父母颜色）
     */
    [[nodiscard]] static DyeColor getDyeColorMixFromParents(
        DyeColor parent1Color, DyeColor parent2Color, math::Random& random);

    /**
     * @brief 吃草动画计时器
     */
    [[nodiscard]] i32 getEatAnimationTimer() const { return m_eatAnimationTimer; }

    /**
     * @brief 设置吃草动画计时器
     */
    void setEatAnimationTimer(i32 timer) { m_eatAnimationTimer = timer; }

    // ========== 静态工具方法 ==========

    /**
     * @brief 获取随机羊毛颜色
     *
     * 概率分布：
     * - 5% 黑色
     * - 5% 灰色
     * - 5% 浅灰色
     * - 3% 棕色
     * - 0.2% 粉色
     * - 81.8% 白色
     */
    [[nodiscard]] static DyeColor getRandomSheepColor(math::Random& random);

    /**
     * @brief 根据染料颜色获取对应的羊毛方块
     * @param color 染料颜色
     * @return 对应的羊毛方块指针，无效颜色返回 nullptr
     */
    [[nodiscard]] static const Block* getWoolBlockByColor(DyeColor color);

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.9f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 1.3f; }

    /**
     * @brief 获取站立时眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.95f * height(); }

private:
    DyeColor m_fleeceColor = DyeColor::White; // 羊毛颜色
    bool m_sheared = false;                   // 是否被剪过
    i32 m_eatAnimationTimer = 0;              // 吃草动画计时器
    i32 m_shearCooldown = 0;                  // 剪毛冷却（ticks）
    entity::ai::goal::EatGrassGoal* m_eatGrassGoal =
        nullptr; // 吃草目标指针，用于同步动画计时器（非拥有指针，生命周期由 GoalSelector 管理）

    static constexpr i32 EAT_GRASS_TIMER_MAX = 40; // 吃草动画持续时间
};

} // namespace mc
