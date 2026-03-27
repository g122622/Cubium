#pragma once

#include "IBlockSource.hpp"
#include "../../../item/ItemStack.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 发射行为接口
 *
 * 定义物品被发射器发射时的行为。
 * 不同物品可以注册不同的发射行为。
 *
 * ## 使用示例
 * ```cpp
 * // 注册发射行为
 * DispenseItemBehaviorRegistry::registerBehavior(Items::ARROW,
 *     std::make_unique<ProjectileDispenseBehavior>(ProjectileType::Arrow));
 *
 * // 执行发射
 * IDispenseItemBehavior* behavior = DispenseItemBehaviorRegistry::getBehavior(stack);
 * if (behavior) {
 *     behavior->dispense(source, stack);
 * }
 * ```
 *
 * 参考: net.minecraft.dispenser.IDispenseItemBehavior
 */
class IDispenseItemBehavior {
public:
    virtual ~IDispenseItemBehavior() = default;

    /**
     * @brief 执行发射行为
     *
     * @param source 发射源（提供位置和世界信息）
     * @param stack 要发射的物品堆（会被修改，如减少数量）
     * @return ItemStack 发射后的物品堆（可能为空或减少数量）
     */
    virtual ItemStack dispense(IBlockSource& source, ItemStack& stack) = 0;

    /**
     * @brief 是否成功发射
     *
     * 某些发射行为可能失败（如打火石没有耐久度）。
     * 失败时不会播放发射音效。
     *
     * @return true 如果成功发射
     */
    [[nodiscard]] virtual bool isSuccess() const { return true; }
};

/**
 * @brief 默认发射行为
 *
 * 从发射器中投掷物品到世界中。
 * 创建一个物品实体，设置速度并生成到世界中。
 *
 * 参考: net.minecraft.dispenser.DefaultDispenseItemBehavior
 */
class DefaultDispenseItemBehavior : public IDispenseItemBehavior {
public:
    ItemStack dispense(IBlockSource& source, ItemStack& stack) override;

protected:
    /**
     * @brief 执行投掷逻辑
     *
     * @param source 发射源
     * @param stack 物品堆
     * @param direction 发射方向
     * @param velocity 发射速度
     * @param inaccuracy 发射偏差
     * @return ItemStack 投掷后的物品堆
     */
    virtual ItemStack doDispense(IBlockSource& source, ItemStack& stack,
                                  Direction direction, f32 velocity = 0.2f, f32 inaccuracy = 6.0f);

    /**
     * @brief 播放发射音效
     * @param source 发射源
     */
    virtual void playSound(IBlockSource& source);

    /**
     * @brief 生成发射粒子
     * @param source 发射源
     */
    virtual void spawnParticles(IBlockSource& source);

    /**
     * @brief 计算发射位置
     *
     * @param source 发射源
     * @param direction 发射方向
     * @return 发射位置（方块出口处）
     */
    [[nodiscard]] static DispensePosition getDispensePosition(IBlockSource& source, Direction direction);
};

/**
 * @brief 可选发射行为基类
 *
 * 某些发射行为可能成功或失败（如打火石点火、桶装满水等）。
 * 失败时不播放发射音效，不减少物品数量。
 *
 * 参考: net.minecraft.dispenser.OptionalDispenseItemBehavior
 */
class OptionalDispenseItemBehavior : public DefaultDispenseItemBehavior {
public:
    OptionalDispenseItemBehavior() : m_success(true) {}

    [[nodiscard]] bool isSuccess() const override { return m_success; }

protected:
    /**
     * @brief 设置发射结果
     * @param success 是否成功
     */
    void setSuccess(bool success) { m_success = success; }

private:
    bool m_success;
};

/**
 * @brief 投掷物发射行为基类
 *
 * 用于发射投掷物（箭矢、雪球、鸡蛋等）。
 *
 * 参考: net.minecraft.dispenser.ProjectileDispenseBehavior
 */
class ProjectileDispenseBehavior : public DefaultDispenseItemBehavior {
public:
    /**
     * @brief 构造函数
     * @param projectileType 投掷物类型（箭矢、雪球、鸡蛋等）
     * @param velocity 发射速度
     * @param inaccuracy 发射偏差
     */
    explicit ProjectileDispenseBehavior(i32 projectileType, f32 velocity = 1.1f, f32 inaccuracy = 6.0f);

    ItemStack dispense(IBlockSource& source, ItemStack& stack) override;

protected:
    /**
     * @brief 获取投掷物类型
     * @return 投掷物类型ID
     */
    [[nodiscard]] i32 getProjectileType() const { return m_projectileType; }

    /**
     * @brief 获取发射速度
     * @return 发射速度
     */
    [[nodiscard]] f32 getVelocity() const { return m_velocity; }

    /**
     * @brief 获取发射偏差
     * @return 发射偏差
     */
    [[nodiscard]] f32 getInaccuracy() const { return m_inaccuracy; }

private:
    i32 m_projectileType;
    f32 m_velocity;
    f32 m_inaccuracy;
};

} // namespace blocks
} // namespace mc
