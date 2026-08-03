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
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ThrowableEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 投掷物品实体基类
 *
 * 用于雪球、鸡蛋、末影珍珠等可投掷物品。
 * 子类只需要实现 getDefaultItem() 和 onImpact()。
 */
class ProjectileItemEntity : public ThrowableEntity {
public:
    ~ProjectileItemEntity() noexcept override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    // ========== 投掷物品方法 ==========

    /**
     * @brief 获取默认物品
     * @return 物品指针
     */
    [[nodiscard]] virtual const Item* getDefaultItem() const = 0;

    /**
     * @brief 获取物品堆
     * @return 物品堆
     */
    [[nodiscard]] ItemStack getItemStack() const { return m_itemStack; }

    /**
     * @brief 设置物品堆
     */
    void setItemStack(const ItemStack& stack) { m_itemStack = stack; }

protected:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    explicit ProjectileItemEntity(EntityInstanceId id);

    // 物品堆
    ItemStack m_itemStack;
};

/**
 * @brief 雪球实体
 *
 * 雪球对烈焰人造成3点伤害，对其他实体无伤害。
 */
class SnowballEntity : public ProjectileItemEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit SnowballEntity(EntityInstanceId id);

    [[nodiscard]] const Item* getDefaultItem() const override;

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onImpact(const RayTraceResult& result) override;
};

/**
 * @brief 鸡蛋实体
 *
 * 鸡蛋有概率孵化出小鸡。
 */
class EggEntity : public ProjectileItemEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit EggEntity(EntityInstanceId id);

    [[nodiscard]] const Item* getDefaultItem() const override;

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onImpact(const RayTraceResult& result) override;

private:
    /**
     * @brief 尝试孵化小鸡
     * @return 是否成功孵化
     */
    bool _tryHatchChicken();
};

/**
 * @brief 末影珍珠实体
 *
 * 末影珍珠会将玩家传送至落点。
 */
class EnderPearlEntity : public ProjectileItemEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit EnderPearlEntity(EntityInstanceId id);

    [[nodiscard]] const Item* getDefaultItem() const override;

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onImpact(const RayTraceResult& result) override;
};

/**
 * @brief 药水实体
 *
 * 投掷型药水和滞留型药水。
 */
class PotionEntity : public ProjectileItemEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit PotionEntity(EntityInstanceId id);

    [[nodiscard]] const Item* getDefaultItem() const override;

    /**
     * @brief 是否为滞留型药水
     */
    [[nodiscard]] bool isLingering() const { return m_lingering; }

    /**
     * @brief 设置是否为滞留型药水
     */
    void setLingering(bool lingering) { m_lingering = lingering; }

protected:
    void onImpact(const RayTraceResult& result) override;

private:
    bool m_lingering = false;
};

/**
 * @brief 经验瓶实体
 *
 * 投掷后破裂并释放经验球。
 */
class ExperienceBottleEntity : public ProjectileItemEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit ExperienceBottleEntity(EntityInstanceId id);

    [[nodiscard]] const Item* getDefaultItem() const override;

    /**
     * @brief 获取经验值
     */
    [[nodiscard]] i32 experience() const { return m_experience; }

    /**
     * @brief 设置经验值
     */
    void setExperience(i32 exp) { m_experience = exp; }

protected:
    void onImpact(const RayTraceResult& result) override;

private:
    i32 m_experience = 0;
};

} // namespace entity
} // namespace mc
