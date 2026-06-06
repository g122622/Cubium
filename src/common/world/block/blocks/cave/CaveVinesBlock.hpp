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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ON AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../IGrowable.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 洞穴藤蔓顶部方块
 *
 * 洞穴藤蔓的生长尖端，可结果，响应随机刻进行生长。
 * 年龄属性控制生长阶段（0-25），浆果属性表示是否结果。
 * 有浆果时发出14级光照，右键可收获发光浆果。
 *
 * 参考: net.minecraft.block.CaveVinesBlock
 */
class CaveVinesBlock : public Block, public IGrowable {
public:
    explicit CaveVinesBlock(const BlockProperties& properties);

    ~CaveVinesBlock() override = default;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 有浆果时发出14级光照
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return state.get(BlockStateProperties::BERRIES()) ? 14 : 0;
    }

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== IGrowable 接口 ==========

    /**
     * @brief 未达到最大年龄时可以生长
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 骨粉对洞穴藤蔓总是有效
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 骨粉直接生长到最大年龄
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== 交互 ==========

    /**
     * @brief 右键收获发光浆果
     *
     * 浆果存在时，右键收获一个发光浆果，浆果属性变为false。
     * 不会破坏方块本身。
     */
    [[nodiscard]] ActionResultType onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;

    /**
     * @brief 最大年龄值
     */
    static constexpr i32 MAX_AGE = 25;

    /**
     * @brief 检查位置是否有浆果（静态辅助方法，供CaveVinesPlantBlock使用）
     */
    static bool _hasBerries(const BlockState& state);
};

} // namespace blocks
} // namespace mc
