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
#include "../../../core/BoostHelper.hpp"
#include "../../../interfaces/IEquipable.hpp"
#include "../../../interfaces/IRideable.hpp"
#include "../basic/AnimalEntity.hpp"
#include <cmath>
#include <memory>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 炽足兽实体
 *
 * 生活在下界的被动生物，可以在熔岩上行走。
 *
 * 特性：
 * - 熔岩行走：可以在熔岩表面行走
 * - 骑乘：可以被玩家骑乘，使用 warped fungus on a stick 控制
 * - 冷却：离开熔岩后会发抖，需要回到熔岩
 * - 繁殖：使用诡异菌繁殖
 * - 乘骑：小炽足兽会骑在成年炽足兽头上
 *
 * 参考 MC 1.16.5 StriderEntity
 */
class StriderEntity : public AnimalEntity, public entity::IRideable, public entity::IEquipable {
public:
    using Entity::canBeRidden;
    using LivingEntity::getEquipment;
    using LivingEntity::setEquipment;

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    StriderEntity(EntityId id);
    ~StriderEntity() override = default;

    // 禁止拷贝
    StriderEntity(const StriderEntity&) = delete;
    StriderEntity& operator=(const StriderEntity&) = delete;

    // 允许移动
    StriderEntity(StriderEntity&&) = delete;
    StriderEntity& operator=(StriderEntity&&) = delete;

    /**
     * @brief 创建炽足兽实体
     * @param world 世界实例
     * @return 新的炽足兽实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 熔岩状态 ==========

    /**
     * @brief 是否在熔岩中
     * MC 1.16.5: 重写以支持站在熔岩表面
     */
    [[nodiscard]] bool isInLava() const override;

    /**
     * @brief 是否在熔岩表面
     */
    [[nodiscard]] bool isOnLavaSurface() const { return m_onLavaSurface; }

    /**
     * @brief 设置是否在熔岩表面
     */
    void setOnLavaSurface(bool surface) { m_onLavaSurface = surface; }

    /**
     * @brief 是否寒冷（不在熔岩中）
     */
    [[nodiscard]] bool isCold() const { return m_coldTimer > 0; }

    /**
     * @brief 获取寒冷计时器
     */
    [[nodiscard]] i32 getColdTimer() const { return m_coldTimer; }

    /**
     * @brief 设置寒冷计时器
     */
    void setColdTimer(i32 timer) { m_coldTimer = timer; }

    // ========== 骑乘系统 (IRideable) ==========

    [[nodiscard]] bool hasSaddle() const override { return m_saddled; }
    void setSaddle(bool saddle) override;
    void onPlayerStartRiding(mc::Player* player) override
    {
        MC_UNUSED(player);
        m_isBeingRidden = true;
    }
    void onPlayerStopRiding(mc::Player* player) override
    {
        MC_UNUSED(player);
        m_isBeingRidden = false;
    }
    [[nodiscard]] f32 getSteeringSpeed() const override;
    bool boost() override;

    /**
     * @brief 是否被骑乘
     */
    [[nodiscard]] bool isBeingRidden() const { return m_isBeingRidden; }

    /**
     * @brief 是否可以骑乘
     */
    [[nodiscard]] bool canBeRidden() const { return true; }

    /**
     * @brief 是否可以在水中骑乘
     * MC 1.16.5: 炽足兽不能在水中骑乘（但可以在熔岩中）
     */
    [[nodiscard]] bool canBeRiddenInWater() const override;

    /**
     * @brief 是否可以被控制方向
     * MC 1.16.5: 需要玩家手持诡异菌钓竿
     */
    [[nodiscard]] bool canBeSteered() const override;

    /**
     * @brief 执行骑乘移动逻辑
     * MC 1.16.5: StriderEntity.travelTowards()
     */
    void travelTowards(const Vector3& travelVec) override;

    // ========== 移动 ==========

    /**
     * @brief 处理移动
     * MC 1.16.5: StriderEntity.travel()
     * 重写以使用 IRideable::ride()
     */
    void travel(const Vector3& travelVec) override;

    // ========== 加速系统 ==========

    /**
     * @brief 是否正在加速
     */
    [[nodiscard]] bool isBoosting() const { return m_boostHelper.isBoosting(); }

    /**
     * @brief 设置加速状态
     */
    void setBoosting(bool boosting) { MC_UNUSED(boosting); /* handled by BoostHelper */ }

    /**
     * @brief 获取加速时间
     */
    [[nodiscard]] i32 getBoostTime() const override { return m_boostHelper.getBoostTime(); }

    /**
     * @brief 设置加速时间
     */
    void setBoostTime(i32 time) override { m_boostHelper.setBoostTime(time); }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 炽足兽使用诡异菌繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.5f : 1.0f; }

    /**
     * @brief 获取乘客骑乘偏移
     * MC 1.16.5: StriderEntity.getMountedYOffset()
     *
     * 参考: net.minecraft.entity.passive.StriderEntity.getMountedYOffset()
     * float f = Math.min(0.25F, this.limbSwingAmount);
     * float f1 = this.limbSwing;
     * return (double)this.getHeight() - 0.19D + (double)(0.12F * MathHelper.cos(f1 * 1.5F) * 2.0F * f);
     *
     * 这个方法产生一个基于步态动画的上下波动效果，模拟炽足兽行走时的起伏。
     */
    [[nodiscard]] f64 getMountedYOffset() const override
    {
        // MC 1.16.5: 计算步态动画参数
        // f = min(0.25, limbSwingAmount) - 限制最大波动幅度
        // f1 = limbSwing - 步态周期计数器
        f32 limbSwingAmountClamped = std::min(0.25f, limbSwingAmount());
        f32 limbSwingValue = limbSwing();

        // MC 1.16.5: 基础高度偏移 = height - 0.19
        // 加上动态波动 = 0.12 * cos(limbSwing * 1.5) * 2.0 * limbSwingAmountClamped
        // 这产生一个基于步态的上下波动效果
        return static_cast<f64>(height()) - 0.19 +
            static_cast<f64>(0.12f * std::cos(limbSwingValue * 1.5f) * 2.0f * limbSwingAmountClamped);
    }

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 死亡时掉落鞍
     * MC 1.16.5 StriderEntity.dropInventory()
     */
    void die(DamageSource& cause) override;

    // ========== IEquipable 接口实现 ==========

    /**
     * @brief 获取装备槽数量
     * MC 1.16.5: 炽足兽只有一个鞍槽
     */
    [[nodiscard]] i32 getEquipmentSlotCount() const override { return 1; }

    /**
     * @brief 获取指定槽位的装备
     * @param slot 槽位索引 (0 = 鞍槽)
     */
    [[nodiscard]] ItemStack getEquipment(i32 slot) const override;

    /**
     * @brief 设置指定槽位的装备
     * @param slot 槽位索引 (0 = 鞍槽)
     */
    void setEquipment(i32 slot, const ItemStack& item) override;

    /**
     * @brief 检查是否可以装备指定物品
     * MC 1.16.5: 炽足兽只能装备鞍
     */
    [[nodiscard]] bool canEquip(const ItemStack& item, i32 slot) const override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 内部方法 ==========

    /**
     * @brief 更新寒冷状态
     * MC 1.16.5: 检查是否在温暖环境中
     */
    void updateColdStatus();

    /**
     * @brief 更新熔岩行走物理
     * MC 1.16.5: func_234318_eL_()
     */
    void updateLavaWalking();

private:
    // 熔岩状态
    bool m_onLavaSurface = false;
    i32 m_coldTimer = 0;

    // 骑乘状态
    bool m_saddled = false;
    bool m_isBeingRidden = false;

    // 加速状态（MC 1.16.5: field_234313_bz_）
    BoostHelper m_boostHelper;
};

} // namespace mc
