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
#include "../../../core/AgeableEntity.hpp"

namespace mc {

// 前向声明
class ItemStack;
class DamageSource;

/**
 * @brief 动物实体基类
 *
 * 可繁殖的动物实体基类，支持喂食、繁殖、跟随父母等行为。
 * 猪、牛、羊、鸡等动物继承此类。
 *
 * 参考 MC 1.16.5 AnimalEntity
 */
class AnimalEntity : public AgeableEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AnimalEntity(LegacyEntityType type, EntityId id);
    ~AnimalEntity() override = default;

    // 禁止拷贝
    AnimalEntity(const AnimalEntity&) = delete;
    AnimalEntity& operator=(const AnimalEntity&) = delete;

    // 允许移动
    AnimalEntity(AnimalEntity&&) = default;
    AnimalEntity& operator=(AnimalEntity&&) = default;

    // ========== 繁殖系统 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * @param itemStack 物品堆
     * @return 是否可以用于繁殖
     *
     * MC 1.16.5: 默认检查是否为小麦
     * 子类应该重写此方法来定义特定的繁殖物品
     */
    [[nodiscard]] virtual bool isBreedingItem(const ItemStack& itemStack) const;

    /**
     * @brief 检查是否可以与另一动物交配
     * @param other 另一个动物
     * @return 是否可以交配
     *
     * MC 1.16.5: 检查双方都是成体、都处于爱心状态、是同类
     */
    [[nodiscard]] virtual bool canMateWith(const AnimalEntity& other) const;

    /**
     * @brief 检查是否可以繁殖
     *
     * MC 1.16.5: 年龄为0且不处于爱心状态
     * 子类可重写以添加额外条件（如海龟检查是否有蛋）
     */
    [[nodiscard]] virtual bool canBreed() const;

    /**
     * @brief 生成幼体
     * @param partner 交配伙伴
     * @return 生成的幼体实体
     *
     * 子类必须重写此方法来创建特定类型的幼体
     */
    virtual std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) = 0;

    // ========== 爱心状态 ==========

    /**
     * @brief 检查是否处于爱心状态
     * MC 1.16.5: isInLove()
     *
     * 注意：继承自 AgeableEntity::isInLove()
     */
    // 使用 AgeableEntity::isInLove()

    /**
     * @brief 获取喂食玩家的UUID
     */
    [[nodiscard]] u64 getLoveCause() const { return m_loveCause; }

    /**
     * @brief 设置喂食玩家
     *
     * MC 1.16.5: 设置爱心状态并记录玩家UUID
     */
    void setInLove(u64 playerUuid = 0);

    /**
     * @brief 重置爱心状态
     *
     * 注意：AgeableEntity::resetLove() 清空爱心计时器
     */
    void resetInLove();

    // ========== 生成和经验 ==========

    /**
     * @brief 获取环境音间隔
     *
     * MC 1.16.5: 返回 120 ticks
     */
    [[nodiscard]] i32 getTalkInterval() const override { return 120; }

    /**
     * @brief 是否可以消失
     *
     * MC 1.16.5: 动物不会消失
     * @param distanceToClosestPlayer 到最近玩家的距离（未使用）
     */
    [[nodiscard]] bool canDespawn(double distanceToClosestPlayer) const override
    {
        (void)distanceToClosestPlayer;
        return false;
    }

    /**
     * @brief 获取经验值
     *
     * MC 1.16.5: 返回 1-3 经验
     */
    [[nodiscard]] i32 getExperiencePoints() const;

    /**
     * @brief 生成爱心粒子
     *
     * MC 1.16.5: 每10tick生成心形粒子
     */
    void spawnHeartParticles();

    // ========== 路径权重 ==========

    /**
     * @brief 获取路径权重
     *
     * MC 1.16.5: AnimalEntity.getBlockPathWeight()
     * - 脚下是草方块: 返回 10.0F
     * - 否则: 返回亮度 - 0.5F
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 路径权重（越高越好）
     */
    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

    // ========== 生命周期 ==========

    void tick() override;

    // ========== 受伤处理 ==========

    /**
     * @brief 受伤处理
     *
     * MC 1.16.5: 动物受伤时清空爱心状态
     */
    bool hurt(DamageSource& source, f32 amount) override;

protected:
    /**
     * @brief 注册 AI 目标
     *
     * 子类应该调用此方法来注册基础动物行为：
     * - SwimGoal (优先级 0)
     * - PanicGoal (优先级 1)
     * - BreedGoal (优先级 2)
     * - TemptGoal (优先级 3)
     * - FollowParentGoal (优先级 4)
     * - WaterAvoidingRandomWalkingGoal (优先级 5)
     * - LookAtGoal (优先级 6)
     * - LookRandomlyGoal (优先级 7)
     */
    void registerGoals() override;

    /**
     * @brief 注册属性
     *
     * 注册动物的基础属性。子类应该调用此方法然后覆盖特定属性值。
     * MC 1.16.5 动物默认属性：
     * - MAX_HEALTH: 10.0
     * - MOVEMENT_SPEED: 0.2
     */
    void registerAttributes() override;

    /**
     * @brief 更新爱心状态
     */
    void updateInLove();

private:
    u64 m_loveCause = 0; // 使其进入爱心状态的玩家UUID
};

} // namespace mc
