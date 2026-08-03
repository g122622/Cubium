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

#include "../growing_plant/GrowingPlantBodyBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IGrowable.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 洞穴藤蔓身体方块
 *
 * 参考 MC 1.21.11: CaveVinesPlantBlock (继承自 GrowingPlantBodyBlock)
 * 向下生长藤蔓的身体部分，不具有独立生长能力。
 * 有浆果时发出14级光照，右键可收获发光浆果。
 * 当上方头部方块被移除时，自动变成新的头部方块。
 */
class CaveVinesPlantBlock : public GrowingPlantBodyBlock, public IGrowable {
public:
    explicit CaveVinesPlantBlock(const BlockProperties& properties);
    ~CaveVinesPlantBlock() override = default;

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

    // ========== GrowingPlantBodyBlock 覆盖 ==========

    [[nodiscard]] const Block* getHeadBlock() const override;
    [[nodiscard]] const Block* getBodyBlock() const override;

    [[nodiscard]] BlockState updateHeadAfterConvertedFromBody(const BlockState& bodyState) const override;

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
     * 洞穴藤蔓身体的中键选取不返回方块物品，而是返回发光浆果物品。
     */
    [[nodiscard]] ItemStack getCloneItemStack(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

} // namespace blocks
} // namespace mc
