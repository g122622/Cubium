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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/Block.hpp"

namespace mc {

// 前向声明
class LivingEntity;

namespace blocks {

/**
 * @brief TNT方块
 *
 * TNT是一种可以被红石信号或火焰点燃的爆炸性方块。
 *
 * ## 特性
 * - 红石触发点燃
 * - 火焰/熔岩点燃
 * - 玩家使用打火石/火焰弹右键点燃
 * - 燃烧投掷物命中点燃
 * - 爆炸连锁反应（通过 onBlockExploded）
 * - 爆炸产生伤害和破坏
 * - 受 tntExplodes 游戏规则控制
 * - 玩家破坏不稳定的TNT时自动点燃
 * - 爆炸时不会掉落物品
 *
 * 参考: net.minecraft.block.TntBlock
 */
class TNTBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit TNTBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 玩家使用物品右键TNT时的交互
     *
     * 当玩家手持打火石或火焰弹右键TNT时，点燃TNT。
     * 如果 tntExplodes 游戏规则为 false，则显示 action bar 消息。
     * 对应 MC Java 的 TntBlock.useItemOn()。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 玩家破坏方块前的回调
     *
     * 如果TNT处于不稳定状态（UNSTABLE=true）且玩家不在创造模式下，
     * 则自动点燃TNT。
     * 对应 MC Java 的 TntBlock.playerWillDestroy()。
     */
    void playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player) override;

    /**
     * @brief 投掷物命中TNT时的回调
     *
     * 当燃烧的投掷物（如箭矢、火球等）命中TNT时，点燃TNT。
     * 对应 MC Java 的 TntBlock.onProjectileHit()。
     */
    void onProjectileHit(
        IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Normal;
    }

    /**
     * @brief TNT方块是否可以在爆炸中掉落物品
     *
     * TNT方块在爆炸中不应掉落物品（对应MC Java的TntBlock.dropFromExplosion返回false）。
     * 当TNT被其他爆炸（如苦力怕）摧毁时，不应产生TNT物品掉落，
     * 因为 onBlockExploded 已经生成了点燃的TNT实体。
     */
    [[nodiscard]] bool canDropFromExplosion(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief TNT 方块被爆炸摧毁时的回调
     *
     * 当其他爆炸摧毁 TNT 方块时调用。如果 tntExplodes 游戏规则为 true，
     * 生成一个随机短引信的点燃 TNT 实体（连锁爆炸）。
     * 通过 explosion 参数获取间接源实体，作为连锁 TNT 的 owner。
     * 对应 MC Java 的 TntBlock.wasExploded()。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @param explosion 引发此方块破坏的爆炸，可能为 nullptr
     */
    void onBlockExploded(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const world::explosion::Explosion* explosion) const override;

    // ========== TNT特有方法 ==========

    /**
     * @brief 检查TNT是否不稳定（即将爆炸）
     * @param state 方块状态
     * @return true 如果不稳定
     */
    [[nodiscard]] static bool isUnstable(const BlockState& state);

    /**
     * @brief 点燃TNT（prime + 移除方块）
     *
     * 调用 prime() 生成点燃的TNT实体并播放音效，成功后移除TNT方块。
     * 如果 tntExplodes 游戏规则为 false，则不会点燃。
     *
     * 对应 MC Java 中先调用 prime() 再调用 setBlock(AIR) 的组合模式。
     * 适用于 onBlockAdded、neighborChanged、onBlockActivated、onProjectileHit 等场景。
     * 不适用于 playerWillDestroy（方块移除由破坏流程处理）。
     *
     * @param world 世界引用
     * @param pos TNT位置
     * @param state 方块状态
     * @return true 如果成功点燃（生成点燃的TNT实体），false 如果游戏规则禁止
     */
    [[nodiscard]] bool ignite(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 点燃TNT（prime + 移除方块，带点燃者）
     *
     * 与 ignite(world, pos, state) 相同，但传递点燃者信息。
     * 点燃者信息会传递给生成的TNT实体，用于伤害归属判定。
     *
     * @param world 世界引用
     * @param pos TNT位置
     * @param state 方块状态
     * @param igniter 点燃者（可能为nullptr）
     * @return true 如果成功点燃（生成点燃的TNT实体），false 如果游戏规则禁止
     */
    [[nodiscard]] bool ignite(IWorld& world, const BlockPos& pos, const BlockState& state, LivingEntity* igniter);

    /**
     * @brief 点燃TNT（仅生成实体和音效，不移除方块）
     *
     * 生成点燃的TNT实体、播放音效、发出 PRIME_FUSE 游戏事件，
     * 但不移除TNT方块。调用方需自行负责移除方块。
     *
     * 对应 MC Java 的 TntBlock.prime() 静态方法。
     * 适用于 playerWillDestroy（方块移除由破坏流程处理）等
     * 不需要本方法移除方块的场景。
     *
     * @param world 世界引用
     * @param pos TNT位置
     * @param igniter 点燃者（可能为nullptr）
     * @return true 如果成功点燃（生成点燃的TNT实体），false 如果游戏规则禁止或客户端
     */
    [[nodiscard]] static bool prime(IWorld& world, const BlockPos& pos, LivingEntity* igniter);

    /**
     * @brief 爆炸TNT
     *
     * 创建爆炸效果并移除方块。
     *
     * @param world 世界引用
     * @param pos TNT位置
     * @param power 爆炸威力
     */
    void explode(IWorld& world, const BlockPos& pos, f32 power);

private:
    /**
     * @brief 检查周围是否有火焰或熔岩
     * @param world 世界引用
     * @param pos TNT位置
     * @return true 如果有火焰源
     */
    [[nodiscard]] bool _hasFlammableNeighbor(IWorld& world, const BlockPos& pos) const;
};

} // namespace blocks
} // namespace mc
