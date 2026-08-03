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

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../blockentity/interactive/BeehiveBlockEntity.hpp"
#include "../../Block.hpp"
#include "../../BlockTags.hpp"
#include "../../Material.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Player;

namespace blocks {

/**
 * @brief 蜂巢/蜂箱方块
 *
 * 蜜蜂居住和产蜜的方块。
 *
 * 状态属性：
 * - HONEY_LEVEL_0_5: 蜂蜜等级 (0-5)
 * - FACING: 朝向
 */
class BeehiveBlock : public Block {
public:
    explicit BeehiveBlock(const BlockProperties& properties);
    ~BeehiveBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getHoneyLevel(const BlockState& state) const;

    /**
     * @brief 在指定状态基础上设置蜂蜜等级
     * @param state 基础状态（保留FACING等其他属性）
     * @param level 目标蜂蜜等级 (0-5)
     */
    [[nodiscard]] BlockState withHoneyLevel(const BlockState& state, i32 level) const;

    [[nodiscard]] i32 getMaxHoneyLevel() const { return 5; }

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 交互 ==========

    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建蜂巢方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 蜂巢逻辑 ==========

    /**
     * @brief 重置蜂蜜等级为0
     * @param world 世界引用
     * @param state 当前方块状态
     * @param pos 方块位置
     */
    void resetHoneyLevel(IWorld& world, const BlockState& state, const BlockPos& pos);

    /**
     * @brief 释放蜜蜂并重置蜂蜜等级
     * @param world 世界引用
     * @param state 当前方块状态
     * @param pos 方块位置
     * @param player 触发释放的玩家（可能为nullptr）
     * @param releaseStatus 释放状态
     */
    void releaseBeesAndResetHoneyLevel(IWorld& world,
        const BlockState& state,
        const BlockPos& pos,
        Player* player,
        blockentity::BeeReleaseStatus releaseStatus);

    /**
     * @brief 掉落蜜脾物品
     * @param world 世界引用
     * @param pos 方块位置
     *
     * 在方块位置掉落3个蜜脾。
     */
    static void dropHoneycomb(IWorld& world, const BlockPos& pos);

    /**
     * @brief 激怒附近的蜜蜂
     * @param world 世界引用
     * @param pos 蜂巢位置
     * @param player 触发的玩家
     */
    static void angerNearbyBees(IWorld& world, const BlockPos& pos, Player& player);

    // ========== 红石 ==========

    /**
     * @brief 是否有比较器输出
     * @return true，蜂巢支持红石比较器
     */
    [[nodiscard]] bool hasAnalogOutputSignal() const noexcept { return true; }

    /**
     * @brief 获取比较器信号强度
     * @param state 方块状态
     * @return 蜂蜜等级 (0-5)
     */
    [[nodiscard]] i32 getAnalogOutputSignal(const BlockState& state) const;
};

} // namespace blocks
} // namespace mc
