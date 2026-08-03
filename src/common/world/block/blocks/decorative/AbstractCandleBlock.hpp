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

#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../IBlockAnimateContext.hpp"
#include "common/util/math/Vector3.hpp"
#include <vector>

namespace mc {

class IWorld;
class IBlockReader;
class Player;
class Entity;

namespace blocks {

/**
 * @brief 蜡烛方块抽象基类
 *
 * 提供蜡烛方块的共享逻辑，包括点燃、熄灭、投掷物交互和粒子动画。
 * 具体的蜡烛方块（普通蜡烛、蛋糕蜡烛）应继承此类并实现 getParticleOffsets()。
 *
 * 状态属性：
 * - LIT: 是否点燃
 * - CANDLES: 蜡烛数量（1-4，仅普通蜡烛使用）
 *
 * 子类必须实现：
 * - getParticleOffsets(): 根据蜡烛数量返回粒子偏移位置
 *
 * 注意：CANDLES 和 CANDLE_CAKES 标签尚未注册到 BlockTags，
 * 暂时在 isLit 中仅检查 LIT 属性。
 */
class AbstractCandleBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit AbstractCandleBlock(BlockProperties properties);

    ~AbstractCandleBlock() override = default;

    // ========== 点燃/熄灭 ==========

    /**
     * @brief 检查蜡烛是否点燃
     *
     * 检查方块状态的 LIT 属性是否为 true。
     * 目前尚未注册 CANDLES/CANDLE_CAKES 标签，仅检查 LIT 属性。
     *
     * @param state 方块状态
     * @return 是否点燃
     */
    [[nodiscard]] static bool isLit(const BlockState& state);

    /**
     * @brief 检查蜡烛是否可以被点燃
     *
     * 当蜡烛未点燃时可以点燃。
     *
     * @param state 方块状态
     * @return 是否可以点燃
     */
    [[nodiscard]] virtual bool canBeLit(const BlockState& state) const;

    /**
     * @brief 熄灭蜡烛
     *
     * 将 LIT 属性设为 false，播放熄灭音效并生成烟雾粒子。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param player 熄灭蜡烛的玩家（可能为 nullptr）
     */
    virtual void extinguish(IWorld& world, const BlockPos& pos, BlockState& state, Player* player);

    /**
     * @brief 设置蜡烛点燃状态
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param lit 是否点燃
     */
    static void setLit(IWorld& world, const BlockPos& pos, const BlockState& state, bool lit);

    // ========== 投掷物交互 ==========

    /**
     * @brief 投掷物击中方块
     *
     * 当燃烧的投掷物击中蜡烛时，如果蜡烛可以被点燃，则点燃蜡烛。
     */
    void onProjectileHit(
        IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile) override;

    // ========== 粒子动画 ==========

    /**
     * @brief 获取粒子偏移位置
     *
     * 子类必须实现此方法，根据蜡烛数量返回对应的粒子偏移位置列表。
     * 每个偏移位置相对于方块原点（左下角），用于在 animateTick 中生成火焰和烟雾粒子。
     *
     * @param state 方块状态
     * @return 粒子偏移位置列表
     */
    [[nodiscard]] virtual std::vector<Vector3f> getParticleOffsets(const BlockState& state) const = 0;

    /**
     * @brief 客户端方块动画 tick
     *
     * 点燃状态下，在每个粒子偏移位置生成烟雾和火焰粒子。
     */
    void animateTick(IBlockAnimateContext& context,
        const BlockPos& pos,
        const BlockState& state,
        math::IRandom& random) const override;

    // ========== 光照 ==========

    /**
     * @brief 获取动态光照等级
     *
     * 点燃时发出光照（由子类定义等级），熄灭时不发光。
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 光照等级
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;
};

} // namespace blocks
} // namespace mc
