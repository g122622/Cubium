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

#include "../../../core/Types.hpp"
#include "../../../entity/entities/vehicle/BoatEntity.hpp"
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

namespace fluid {
class Fluid;
}

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
 * IDispenseItemBehavior* behavior = DispenseItemBehaviorRegistry::getBehavior(stack);
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
    virtual ItemStack dispense(IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack) = 0;

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
 */
class DefaultDispenseItemBehavior : public IDispenseItemBehavior {
public:
    ItemStack dispense(IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack) override;

    /**
     * @brief 计算发射位置
     *
     * 计算物品从发射器射出的位置。
     * 发射位置 = 方块中心 + 方向偏移 * 0.7
     *
     * @param pos 发射器位置
     * @param direction 发射方向
     * @return 发射位置（方块出口处，带偏移）
     */
    [[nodiscard]] static Vector3 getDispensePosition(const BlockPos& pos, Direction direction);

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
    virtual ItemStack _doDispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& stack,
        Direction direction,
        f32 speed = 6.0f,
        f32 inaccuracy = 6.0f);

    /**
     * @brief 播放发射音效
     * @param world 世界引用
     * @param pos 发射器位置
     */
    virtual void _playSound(IWorld& world, const BlockPos& pos);

    /**
     * @brief 生成发射粒子
     * @param world 世界引用
     * @param pos 发射器位置
     * @param direction 发射方向
     */
    virtual void _spawnParticles(IWorld& world, const BlockPos& pos, Direction direction);

    /// 发射位置偏移量（从方块中心到出口的距离）
    static constexpr f32 DISPENSE_OFFSET = 0.7f;

    /// Y轴方向发射时的额外偏移
    static constexpr f32 Y_AXIS_OFFSET = 0.125f;

    /// 水平方向发射时的Y轴额外偏移
    static constexpr f32 HORIZONTAL_Y_OFFSET = 0.15625f;

    /// 高斯随机因子的系数
    static constexpr f32 GAUSSIAN_FACTOR = 0.0075f;

    /// 基础速度的最小值
    static constexpr f32 BASE_VELOCITY_MIN = 0.2f;

    /// 基础速度的随机范围
    static constexpr f32 BASE_VELOCITY_RANGE = 0.1f;

    /// Y方向基础速度
    static constexpr f32 Y_VELOCITY_BASE = 0.2f;

    /// 默认拾取延迟（ticks）
    static constexpr i32 DEFAULT_PICKUP_DELAY = 10;
};

/**
 * @brief 可选发射行为基类
 *
 * 某些发射行为可能成功或失败（如打火石点火、桶装满水等）。
 * 失败时不播放发射音效，不减少物品数量。
 */
class OptionalDispenseItemBehavior : public DefaultDispenseItemBehavior {
public:
    OptionalDispenseItemBehavior()
        : m_success(true)
    {}

    [[nodiscard]] bool isSuccess() const override { return m_success; }

protected:
    /**
     * @brief 设置发射结果
     * @param success 是否成功
     */
    void _setSuccess(bool success) { m_success = success; }

    /**
     * @brief 播放音效（根据成功/失败播放不同音效）
     */
    void _playSound(IWorld& world, const BlockPos& pos) override;

    /**
     * @brief 生成粒子（只有成功时才生成）
     */
    void _spawnParticles(IWorld& world, const BlockPos& pos, Direction direction) override;

private:
    bool m_success;
};

/**
 * @brief 投掷物发射行为基类
 *
 * 用于发射投掷物（箭矢、雪球、鸡蛋等）。
 * 通过工厂函数创建具体的投掷物实体。
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
    using ProjectileFactory = std::function<std::unique_ptr<mc::Entity>(IWorld&, const Vector3&, const ItemStack&)>;

    /**
     * @brief 构造函数
     * @param createProjectile 投掷物创建工厂函数
     * @param velocity 发射速度（默认1.1，与MC一致）
     * @param inaccuracy 发射偏差（默认6.0，与MC一致）
     */
    explicit ProjectileDispenseBehavior(ProjectileFactory createProjectile, f32 velocity = 1.1f, f32 inaccuracy = 6.0f);

    ItemStack dispense(IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack) override;

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

/**
 * @brief 船发射行为
 *
 * 在水面放置船实体。
 * 如果目标位置不是水，则作为普通物品发射。
 */
class BoatDispenseBehavior : public DefaultDispenseItemBehavior {
public:
    /**
     * @brief 构造函数
     * @param type 船的类型（木材种类）
     */
    explicit BoatDispenseBehavior(entity::BoatEntity::Type type);

    ItemStack dispense(IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack) override;

private:
    entity::BoatEntity::Type m_boatType;
};

/**
 * @brief 桶发射行为（装满流体的桶）
 *
 * 放置流体到世界中。
 */
class BucketDispenseBehavior : public OptionalDispenseItemBehavior {
public:
    /**
     * @brief 构造函数
     * @param fluid 要放置的流体
     */
    explicit BucketDispenseBehavior(fluid::Fluid& fluid);

    ItemStack dispense(IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack) override;

private:
    fluid::Fluid* m_fluid;
};

/**
 * @brief 空桶发射行为（收集流体）
 *
 * 从世界中收集流体到桶中。
 */
class EmptyBucketDispenseBehavior : public OptionalDispenseItemBehavior {
public:
    EmptyBucketDispenseBehavior() = default;

    ItemStack dispense(IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack) override;
};

/**
 * @brief 打火石发射行为
 *
 * 点燃发射器前方的方块。
 */
class FlintAndSteelDispenseBehavior : public OptionalDispenseItemBehavior {
public:
    FlintAndSteelDispenseBehavior() = default;

    ItemStack dispense(IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack) override;
};

/**
 * @brief 骨粉发射行为
 *
 * 对发射器前方的方块使用骨粉催熟效果。
 */
class BonemealDispenseBehavior : public OptionalDispenseItemBehavior {
public:
    BonemealDispenseBehavior() = default;

    ItemStack dispense(IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack) override;
};

} // namespace blocks
} // namespace mc
