/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, WHETHER
 * ARISING FROM, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../Block.hpp"
#include "../../IBucketPickupHandler.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 细雪方块
 *
 * 一种类似雪的方块，实体陷入其中会缓慢移动。
 * 没有碰撞箱，玩家和生物可以穿过。
 * 空桶可以从细雪方块中提取细雪，返回细雪桶。
 *
 * 参考: net.minecraft.block.PowderSnowBlock
 */
class PowderSnowBlock : public Block, public IBucketPickupHandler {
public:
    explicit PowderSnowBlock(const BlockProperties& properties);

    ~PowderSnowBlock() override = default;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 从细雪方块中取出流体
     *
     * 细雪不是流体，此方法始终返回 nullptr。
     * 使用 pickupItem 代替。
     */
    [[nodiscard]] fluid::Fluid* pickupFluid(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 从细雪方块中提取细雪，返回细雪桶物品
     *
     * 将细雪方块替换为空气，并返回细雪桶物品。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return 细雪桶物品指针，如果无法拾取则返回 nullptr
     */
    [[nodiscard]] const Item* pickupItem(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 获取从细雪方块拾取时播放的音效
     *
     * 返回 ITEM_BUCKET_FILL_POWDER_SNOW 音效。
     */
    [[nodiscard]] const ResourceLocation* getPickupSound(
        IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 实体与细雪方块碰撞时的处理
     *
     * 当实体进入细雪方块时：
     * - 设置实体的 isInPowderSnow 标志
     * - 如果实体可以冰冻，增加冰冻计时器（+1/tick，上限为 getTicksRequiredToFreeze()）
     * - 设置运动减速乘数（0.9, 0.9, 0.9），使实体在细雪中缓慢移动
     *
     * 对应 MC Java 的 PowderSnowBlock.entityInside() + InsideBlockEffectType.FREEZE。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param entity 实体
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;
};

} // namespace blocks
} // namespace mc
