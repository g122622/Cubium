#pragma once

#include "../../../core/Types.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../util/math/Vector3.hpp"
#include <functional>
#include <memory>

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;
enum class Direction : u8;
class Entity;

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
 * DispenseItemBehaviorRegistry::registerBehavior("minecraft:snowball",
 *     std::make_unique<ProjectileDispenseBehavior>(createSnowball, 1.1f, 6.0f));
 *
 * // 执行发射
 * IDispenseItemBehavior* behavior = DispenseItemBehaviorRegistry::instance().getBehavior(stack);
 * if (behavior) {
 *     behavior->dispense(world, pos, state, stack);
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
     * @param world 世界引用
     * @param pos 发射器方块位置
     * @param state 发射器方块状态
     * @param stack 要发射的物品堆（会被修改，如减少数量）
     * @return ItemStack 发射后的物品堆（可能为空或减少数量）
     */
    virtual ItemStack dispense(IWorld& world, const BlockPos& pos,
                               const BlockState& state, ItemStack& stack) = 0;

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
    ItemStack dispense(IWorld& world, const BlockPos& pos,
                       const BlockState& state, ItemStack& stack) override;

protected:
    /**
     * @brief 执行投掷逻辑
     *
     * @param world 世界引用
     * @param pos 发射器位置
     * @param state 发射器方块状态
     * @param stack 物品堆
     * @param direction 发射方向
     * @param speed 发射速度（MC默认为6.0）
     * @param inaccuracy 发射偏差（MC默认为6.0，用于高斯扰动）
     * @return ItemStack 投掷后的物品堆
     */
    virtual ItemStack doDispense(IWorld& world, const BlockPos& pos, const BlockState& state,
                                  ItemStack& stack, Direction direction,
                                  f32 speed = 6.0f, f32 inaccuracy = 6.0f);

    /**
     * @brief 播放发射音效
     * @param world 世界引用
     * @param pos 发射器位置
     */
    virtual void playSound(IWorld& world, const BlockPos& pos);

    /**
     * @brief 生成发射粒子
     * @param world 世界引用
     * @param pos 发射器位置
     * @param direction 发射方向
     */
    virtual void spawnParticles(IWorld& world, const BlockPos& pos, Direction direction);

    /**
     * @brief 计算发射位置
     *
     * @param pos 发射器位置
     * @param direction 发射方向
     * @return 发射位置（方块出口处，带偏移）
     */
    [[nodiscard]] static Vector3 getDispensePosition(const BlockPos& pos, Direction direction);
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

    /**
     * @brief 播放音效（根据成功/失败播放不同音效）
     */
    void playSound(IWorld& world, const BlockPos& pos) override;

    /**
     * @brief 生成粒子（只有成功时才生成）
     */
    void spawnParticles(IWorld& world, const BlockPos& pos, Direction direction) override;

private:
    bool m_success;
};

/**
 * @brief 投掷物发射行为基类
 *
 * 用于发射投掷物（箭矢、雪球、鸡蛋等）。
 * 通过工厂函数创建具体的投掷物实体。
 *
 * 参考: net.minecraft.dispenser.ProjectileDispenseBehavior
 */
class ProjectileDispenseBehavior : public DefaultDispenseItemBehavior {
public:
    /**
     * @brief 投掷物创建函数类型
     *
     * @param world 世界引用
     * @param pos 发射位置
     * @param stack 物品堆（可能包含药水效果等信息）
     * @return 创建的投掷物实体
     */
    using ProjectileFactory = std::function<std::unique_ptr<mc::Entity>(
        IWorld&, const Vector3&, const ItemStack&)>;

    /**
     * @brief 构造函数
     * @param createProjectile 投掷物创建工厂函数
     * @param velocity 发射速度（默认1.1，与MC一致）
     * @param inaccuracy 发射偏差（默认6.0，与MC一致）
     */
    explicit ProjectileDispenseBehavior(
        ProjectileFactory createProjectile,
        f32 velocity = 1.1f,
        f32 inaccuracy = 6.0f);

    ItemStack dispense(IWorld& world, const BlockPos& pos,
                       const BlockState& state, ItemStack& stack) override;

protected:
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
    ProjectileFactory m_createProjectile;
    f32 m_velocity;
    f32 m_inaccuracy;
};

} // namespace blocks
} // namespace mc
