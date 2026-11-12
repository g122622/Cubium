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

#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/blocks/decorative/SkullType.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"

#include <memory>

namespace mc {

class Entity;

namespace blocks {

/**
 * @brief 头颅方块抽象基类
 *
 * 持有 POWERED 属性（被红石信号激活时龙首/猪灵头做张嘴动画，由方块实体 tick 驱动）。
 * 子类：SkullBlock（站立）、WallSkullBlock（贴墙）。
 *
 */
class AbstractSkullBlock : public Block {
public:
    /**
     * @brief 构造头颅方块
     * @param type 头颅种类
     * @param properties 方块属性
     */
    AbstractSkullBlock(SkullType type, const BlockProperties& properties);

    ~AbstractSkullBlock() override = default;

    /**
     * @brief 获取头颅种类
     */
    [[nodiscard]] SkullType getSkullType() const noexcept
    {
        return m_type;
    }

    /**
     * @brief 头颅方块持有方块实体（存储玩家档案/自定义名称/音符盒音色）
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 邻居方块更新：红石信号变化时翻转 POWERED
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

protected:
    /**
     * @brief 头颅种类对应的音符盒乐器（对齐 vanilla 头颅注册的 instrument 属性）
     */
    [[nodiscard]] static NoteBlockInstrument _instrumentForSkullType(SkullType type) noexcept;

    SkullType m_type;
    /// 头颅在三种寻路类型下都不可通行。
    [[nodiscard]] bool isPathfindable(const BlockState& state, PathComputationType type) const override;
};

} // namespace blocks
} // namespace mc
