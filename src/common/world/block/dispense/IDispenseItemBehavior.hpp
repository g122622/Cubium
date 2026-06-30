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
class IInventory;

namespace fluid {
class Fluid;
}

namespace item {
class ProjectileItem;
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
 *     behavior->dispense(world, pos, state, stack, inventory);
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
     * @param dispenserInventory 发射器库存指针（可空，用于consumeWithRemainder放回替换物品）
     * @return ItemStack 发射后的物品堆（可能为空或减少数量）
     */
    virtual ItemStack dispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& stack,
        IInventory* dispenserInventory) = 0;

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
    ItemStack dispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& stack,
        IInventory* dispenserInventory) override;

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

    /**
     * @brief 消耗物品并处理容器物品替换
     *
     * 参考 MC 原版 DefaultDispenseItemBehavior.consumeWithRemainder。
     * 当发射行为消耗一个物品后产生替换物品（如水桶→空桶）时使用。
     *
     * 逻辑：
     * 1. 将原始物品减1 (shrink(1))
     * 2. 如果原始物品变为空（只有1个），直接返回替换物品，替换物品会被写回发射器原槽位
     * 3. 如果原始物品还有剩余（有多个），尝试将替换物品放回发射器库存：
     *    - 放回成功则返回剩余的原始物品
     *    - 库存满了则将替换物品弹出到世界中，并播放默认音效和粒子
     *
     * @param world 世界引用
     * @param pos 发射器位置
     * @param state 发射器方块状态
     * @param original 原始物品堆（会被修改，shrink(1)）
     * @param replacement 替换物品堆（如空桶）
     * @param dispenserInventory 发射器库存指针（可空，为空时替换物品直接弹出）
     * @return ItemStack 应写回发射器原槽位的物品堆
     */
    static ItemStack consumeWithRemainder(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& original,
        const ItemStack& replacement,
        IInventory* dispenserInventory);

    /**
     * @brief 尝试将物品放回发射器库存，放不下则弹出到世界
     *
     * 参考 MC 原版 DefaultDispenseItemBehavior.addToInventoryOrDispense。
     * 先尝试将物品插入发射器库存，如果库存满了则弹出到世界中。
     *
     * @param world 世界引用
     * @param pos 发射器位置
     * @param state 发射器方块状态
     * @param stack 要放回的物品堆
     * @param dispenserInventory 发射器库存指针（可空，为空时直接弹出）
     */
    static void addToInventoryOrDispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const ItemStack& stack,
        IInventory* dispenserInventory);

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

    /**
     * @brief 将物品弹出到世界中
     *
     * 在发射位置创建物品实体并设置速度。
     * 用于发射器行为中需要将替换物品（如空桶、满桶）弹出的场景。
     *
     * @param world 世界引用
     * @param pos 发射器位置
     * @param direction 发射方向
     * @param itemStack 要弹出的物品堆
     */
    static void _spawnItemEntity(IWorld& world, const BlockPos& pos, Direction direction, const ItemStack& itemStack);

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
 * @brief 投掷物发射行为
 *
 * 通过 ProjectileItem 接口创建并发射投掷物。
 * 当物品实现了 ProjectileItem 接口时，发射器自动使用此类进行发射，
 * 无需为每种投掷物单独编写 lambda 工厂函数。
 *
 * 参考: net.minecraft.core.dispenser.ProjectileDispenseBehavior
 */
class ProjectileDispenseBehavior : public DefaultDispenseItemBehavior {
public:
    /**
     * @brief 构造函数
     * @param item 实现了 ProjectileItem 接口的物品引用
     */
    explicit ProjectileDispenseBehavior(const item::ProjectileItem& projectileItem);

    ItemStack dispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& stack,
        IInventory* dispenserInventory) override;

private:
    const item::ProjectileItem& m_projectileItem;
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

    ItemStack dispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& stack,
        IInventory* dispenserInventory) override;

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

    ItemStack dispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& stack,
        IInventory* dispenserInventory) override;

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

    ItemStack dispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& stack,
        IInventory* dispenserInventory) override;
};

/**
 * @brief 打火石发射行为
 *
 * 点燃发射器前方的方块。
 */
class FlintAndSteelDispenseBehavior : public OptionalDispenseItemBehavior {
public:
    FlintAndSteelDispenseBehavior() = default;

    ItemStack dispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& stack,
        IInventory* dispenserInventory) override;
};

/**
 * @brief 骨粉发射行为
 *
 * 对发射器前方的方块使用骨粉催熟效果。
 */
class BonemealDispenseBehavior : public OptionalDispenseItemBehavior {
public:
    BonemealDispenseBehavior() = default;

    ItemStack dispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& stack,
        IInventory* dispenserInventory) override;
};

/**
 * @brief TNT 发射行为
 *
 * 发射器发射 TNT 物品时，生成一个已点燃的 TNT 实体。
 * 如果 tntExplodes 游戏规则为 false，则发射失败（物品不被消耗）。
 * 对应 MC Java 的 DispenseItemBehavior 中 Blocks.TNT 的发射行为。
 */
class TNTDispenseBehavior : public OptionalDispenseItemBehavior {
public:
    TNTDispenseBehavior() = default;

    ItemStack dispense(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        ItemStack& stack,
        IInventory* dispenserInventory) override;
};

} // namespace blocks
} // namespace mc
