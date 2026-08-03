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
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {

// Forward declarations
class ItemStack;
class IWorld;
class Entity;
class BlockPos;

/**
 * @brief 物品掉落工具类
 *
 * 提供统一的物品实体生成方法，封装随机速度计算逻辑。
 *
 * 用法示例:
 * @code
 * // 在实体位置生成单个物品（用于剪羊毛等）
 * ItemDropHelper::spawnItemEntity(world, stack, entity->x(), entity->y(), entity->z(), rng);
 *
 * // 在方块位置生成多个物品（用于方块掉落）
 * ItemDropHelper::spawnItemEntities(world, pos, drops, throwerUuid);
 *
 * // 获取随机速度向量
 * Vector3 velocity = ItemDropHelper::getBlockDropVelocity(rng);
 * @endcode
 */
class ItemDropHelper {
public:
    // ========== 常量 ==========

    /// 默认拾取延迟（ticks）= 0.5秒
    static constexpr i32 DEFAULT_PICKUP_DELAY = 10;

    /// 默认存活时间（ticks）= 5分钟
    static constexpr i32 DEFAULT_LIFETIME = 6000;

    // ========== 随机速度计算 ==========

    /**
     * @brief 获取方块掉落式的随机速度
     *
     * 用于方块破坏后的物品掉落。
     *
     * 速度公式：
     * - X: (random - 0.5) * 0.1 + random * 0.2
     * - Y: random * 0.2
     * - Z: (random - 0.5) * 0.1 + random * 0.2
     *
     * @param rng 随机数生成器
     * @return 随机速度向量
     */
    [[nodiscard]] static Vector3 getBlockDropVelocity(math::Random& rng);

    /**
     * @brief 获取简单随机速度
     *
     * 用于实体丢弃物品等简单场景。
     *
     * 速度公式：
     * - X: random * 0.2 - 0.1  => 范围 [-0.1, 0.1]
     * - Y: 0.2
     * - Z: random * 0.2 - 0.1  => 范围 [-0.1, 0.1]
     *
     * @param rng 随机数生成器
     * @return 随机速度向量
     */
    [[nodiscard]] static Vector3 getSimpleDropVelocity(math::Random& rng);

    /**
     * @brief 获取玩家丢弃物品的速度
     *
     * @param rng 随机数生成器
     * @param dropAround 是否向四周散射（Q键丢弃 vs Ctrl+Q丢弃）
     * @param yaw 玩家朝向（仅 dropAround=false 时使用）
     * @param pitch 玩家俯仰角（仅 dropAround=false 时使用）
     * @return 随机速度向量
     */
    [[nodiscard]] static Vector3 getPlayerDropVelocity(
        math::Random& rng, bool dropAround, f32 yaw = 0.0f, f32 pitch = 0.0f);

    /**
     * @brief 获取高斯分布的随机速度
     *
     * 用于发射器等需要更自然散射的场景。
     *
     * @param rng 随机数生成器
     * @param baseVelocity 基础速度
     * @param inaccuracy 不精确度（标准差因子）
     * @return 随机速度向量（只有 X/Z 有高斯偏移）
     */
    [[nodiscard]] static Vector3 getGaussianVelocity(math::Random& rng, f32 baseVelocity, f32 inaccuracy);

    // ========== 物品实体生成 ==========

    /**
     * @brief 在指定位置生成单个物品实体
     *
     * 使用方块掉落式的随机速度。
     *
     * @param world 世界指针
     * @param stack 物品堆
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param rng 随机数生成器
     * @param pickupDelay 拾取延迟（默认10 ticks）
     * @param ownerUuid 所有者UUID（可选，防止立即拾取）
     * @return 生成的物品实体指针，失败返回 nullptr
     */
    static ItemEntity* spawnItemEntity(IWorld* world,
        const ItemStack& stack,
        f64 x,
        f64 y,
        f64 z,
        math::Random& rng,
        i32 pickupDelay = DEFAULT_PICKUP_DELAY,
        const std::string& ownerUuid = "");

    /**
     * @brief 在指定位置生成物品实体（指定速度）
     *
     * @param world 世界指针
     * @param stack 物品堆
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param vx X方向速度
     * @param vy Y方向速度
     * @param vz Z方向速度
     * @param pickupDelay 拾取延迟
     * @param ownerUuid 所有者UUID
     * @return 生成的物品实体指针
     */
    static ItemEntity* spawnItemEntity(IWorld* world,
        const ItemStack& stack,
        f64 x,
        f64 y,
        f64 z,
        f32 vx,
        f32 vy,
        f32 vz,
        i32 pickupDelay = DEFAULT_PICKUP_DELAY,
        const std::string& ownerUuid = "");

    /**
     * @brief 在实体位置生成物品实体
     *
     * @param entity 实体指针（用于获取位置和世界）
     * @param stack 物品堆
     * @param offsetY Y轴偏移（默认0.5）
     * @param rng 随机数生成器
     * @param pickupDelay 拾取延迟
     * @return 生成的物品实体指针
     */
    static ItemEntity* spawnItemAtEntity(
        Entity* entity, const ItemStack& stack, f32 offsetY, math::Random& rng, i32 pickupDelay = DEFAULT_PICKUP_DELAY);

    /**
     * @brief 在方块位置生成多个物品实体
     *
     * 在方块中心位置生成物品，使用方块掉落式的随机速度。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param drops 掉落物列表
     * @param rng 随机数生成器
     * @param throwerUuid 投掷者UUID
     * @return 生成的实体ID列表
     */
    static std::vector<EntityInstanceId> spawnItemEntities(IWorld* world,
        const BlockPos& pos,
        const std::vector<ItemStack>& drops,
        math::Random& rng,
        const std::string& throwerUuid = "");

private:
    ItemDropHelper() = delete; // 静态工具类，禁止实例化
};

} // namespace mc
