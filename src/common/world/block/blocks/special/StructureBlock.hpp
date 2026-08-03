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
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/GameMasterBlock.hpp"

namespace mc {

class IWorld;
class Player;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 结构方块
 *
 * 用于保存和加载结构的方块。
 * 创造模式和管理员可以使用。
 *
 * 状态属性：
 * - MODE: 结构方块模式 (SAVE, LOAD, CORNER, DATA)
 */
class StructureBlock : public Block, public GameMasterBlock {
public:
    /// 结构方块模式别名，使用 BlockStateProperties 中定义的 StructureMode
    using Mode = BlockStateProperties::StructureMode;

    explicit StructureBlock(const BlockProperties& properties);
    ~StructureBlock() override = default;

    // ========== 标记接口 ==========

    [[nodiscard]] bool isGameMaster() const noexcept override { return true; }

    // ========== 状态属性 ==========

    /**
     * @brief 获取结构方块模式
     *
     * @param state 方块状态
     * @return 当前模式
     */
    [[nodiscard]] Mode getMode(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置时的方块状态
     *
     * 放置时默认设置为 DATA 模式。
     *
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 交互 ==========

    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;
};

} // namespace blocks
} // namespace mc
