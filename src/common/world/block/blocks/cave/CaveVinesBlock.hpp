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
 */

#pragma once

#include "../growing_plant/GrowingPlantHeadBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 洞穴藤蔓顶部方块（生长尖端）
 *
 * 参考 MC 1.21.11: CaveVinesBlock (继承自 GrowingPlantHeadBlock)
 * 向下生长的藤蔓尖端，具有 AGE_0_25 和 BERRIES 属性。
 * 有浆果时发出14级光照，右键可收获发光浆果。
 *
 * MC 1.21.11 对齐修正：
 * - 生长概率 10%（原实现为 11.1% / 1/9）
 * - 新生长藤蔓 11% 概率有浆果（CHANCE_OF_BERRIES_ON_GROWTH = 0.11）
 * - 骨粉效果：设置 BERRIES=true（不是设置 AGE=25）
 * - 继承 GrowingPlantHeadBlock：自动获得 isValidPosition + updatePostPlacement + IGrowable
 */
class CaveVinesBlock : public GrowingPlantHeadBlock {
public:
    explicit CaveVinesBlock(const BlockProperties& properties);
    ~CaveVinesBlock() override = default;

    // ========== 形状与光照 ==========

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return state.get(BlockStateProperties::BERRIES()) ? 14 : 0;
    }

    // ========== GrowingPlantHeadBlock 覆盖 ==========

    [[nodiscard]] const Block* getHeadBlock() const override;
    [[nodiscard]] const Block* getBodyBlock() const override;

    [[nodiscard]] BlockState getGrowIntoState(
        IWorld& world, const BlockPos& pos, BlockState& currentState, math::IRandom& random) override;

    [[nodiscard]] BlockState updateBodyAfterConvertedFromHead(const BlockState& headState) const override;

    // ========== IGrowable 接口 ==========

    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== 交互 ==========

    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 中键选取方块时返回发光浆果
     *
     * 洞穴藤蔓的中键选取不返回方块物品，而是返回发光浆果物品。
     */
    [[nodiscard]] ItemStack getCloneItemStack(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    /** 新生长藤蔓有浆果的概率 (MC 1.21.11: 0.11) */
    static constexpr f32 CHANCE_OF_BERRIES_ON_GROWTH = 0.11f;
};

} // namespace blocks
} // namespace mc
