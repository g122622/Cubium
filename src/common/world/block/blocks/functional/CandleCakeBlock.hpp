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
#include "../decorative/AbstractCandleBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <vector>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Player;
class BlockRaycastResult;

namespace blocks {

/**
 * @brief 蜡烛蛋糕方块
 *
 * 蜡烛插在蛋糕上的装饰性方块，可以点燃/熄灭。
 * 玩家右键可以吃蛋糕（消耗蛋糕，留下蜡烛）。
 * 被投掷物击中时如果投掷物着火会点燃蜡烛。
 *
 * 状态属性：
 * - LIT: 是否点燃
 *
 * 注意：蜡烛蛋糕不继承 CakeBlock，因为它只有 LIT 属性（没有 BITES），
 * 且行为与蛋糕不同（食用后掉落蜡烛方块而非消失）。
 */
class CandleCakeBlock : public AbstractCandleBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param candleBlock 关联的蜡烛方块（食用蛋糕后放置此方块）
     */
    explicit CandleCakeBlock(BlockProperties properties, Block* candleBlock);

    ~CandleCakeBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 光照 ==========

    /**
     * @brief 获取动态光照等级
     *
     * 点燃时亮度为3，熄灭时不发光。
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 粒子 ==========

    [[nodiscard]] std::vector<Vector3f> getParticleOffsets(const BlockState& state) const override;

    // ========== 红石 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键交互
     *
     * - 空手点击蜡烛部分（上半部）且已点燃 → 熄灭
     * - 其他情况 → 吃蛋糕，转化为一片普通蛋糕并掉落蜡烛物品
     *
     * 点燃（打火石/火焰弹）由物品自身处理，返回 Pass 让物品处理。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 静态工具方法 ==========

    /**
     * @brief 检查蜡烛蛋糕是否可以被点燃
     *
     * 检查方块状态是否包含 LIT 属性且未点燃。
     *
     * @param state 方块状态
     * @return 是否可点燃
     */
    [[nodiscard]] static bool canLight(const BlockState& state);

    // ========== 关联蜡烛 ==========

    /**
     * @brief 获取关联的蜡烛方块
     */
    [[nodiscard]] Block* getCandleBlock() const { return m_candleBlock; }

private:
    /// 关联的蜡烛方块（食用蛋糕后放置此方块）
    Block* m_candleBlock;
    /// 蜡烛蛋糕的碰撞形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
