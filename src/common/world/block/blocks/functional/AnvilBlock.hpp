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

#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/blocks/FallingBlock.hpp"

namespace mc {

class BlockState;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 铁砧方块
 *
 * 铁砧是可下落的水平方向方块，落地时会对碰撞箱内的实体造成伤害，
 * 并有概率损坏（anvil → chipped_anvil → damaged_anvil → 消失）。
 *
 * 三个铁砧变体（anvil、chipped_anvil、damaged_anvil）均为 AnvilBlock 实例，
 * 通过不同的 Block 注册来区分。
 *
 * 参考: net.minecraft.world.level.block.AnvilBlock
 */
class AnvilBlock : public FallingBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit AnvilBlock(const BlockProperties& properties);

    ~AnvilBlock() override = default;

    // ========== FallingBlock 回调覆盖 ==========

    /**
     * @brief 开始下落时设置伤害参数
     *
     * 铁砧下落时设置 hurtEntities=true，每格伤害 2.0，最大伤害 40。
     */
    void onStartFalling(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity) override;

    /**
     * @brief 落地成功放置时播放音效
     *
     * 播放铁砧落地音效 (WorldEvents::ANVIL_LAND_SOUND = 1031)。
     */
    void onEndFalling(IWorld& world,
        const BlockPos& pos,
        const BlockState& fallingState,
        const BlockState& hitState,
        entity::FallingBlockEntity& entity) override;

    /**
     * @brief 破碎时播放音效
     *
     * 当铁砧无法放置时播放破坏音效 (WorldEvents::ANVIL_DESTROYED_SOUND = 1029)。
     */
    void onBroken(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity) override;

    // ========== 方块行为覆盖 ==========

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据玩家水平朝向设置 HORIZONTAL_FACING。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 旋转方块状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 静态工具方法 ==========

    /**
     * @brief 铁砧损坏状态转换
     *
     * anvil → chipped_anvil → damaged_anvil → nullptr（完全损坏）
     * 保留朝向属性。
     *
     * @param state 当前铁砧方块状态
     * @return 损坏后的方块状态指针，如果已完全损坏则返回 nullptr
     */
    [[nodiscard]] static const BlockState* damageAnvil(const BlockState& state);
};

} // namespace blocks
} // namespace mc
