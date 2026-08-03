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
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "world/block/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 音符盒方块
 *
 * 音符盒是一个可以发出不同音调的方块，通过红石或玩家右键触发。
 *
 * ## 特性
 * - 25个音阶（0-24）
 * - 音调取决于上方方块类型（乐器）
 * - 红石触发或玩家右键触发
 * - 发出声音并产生粒子效果
 */
class NoteBlock : public Block {
public:
    /// 音阶数量
    static constexpr i32 NOTE_RANGE = 25;

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit NoteBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 音符盒特有方法 ==========

    /**
     * @brief 获取当前音调
     * @param state 方块状态
     * @return i32 音调值（0-24）
     */
    [[nodiscard]] static i32 getNote(const BlockState& state);

    /**
     * @brief 设置音调
     * @param state 方块状态
     * @param note 音调值（0-24）
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withNote(BlockState state, i32 note);

    /**
     * @brief 增加音调（循环）
     * @param state 方块状态
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState cycleNote(BlockState state);

    /**
     * @brief 触发播放音符
     *
     * 根据上方方块类型决定乐器，发出声音和粒子效果。
     *
     * @param world 世界引用
     * @param pos 音符盒位置
     * @param state 当前方块状态
     */
    void triggerNote(IWorld& world, const BlockPos& pos, const BlockState& state);

private:
    /**
     * @brief 根据上方方块类型获取乐器类型
     * @param world 世界引用
     * @param pos 音符盒位置
     * @return 乐器类型ID
     */
    [[nodiscard]] i32 _getInstrumentType(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 播放音符声音
     * @param world 世界引用
     * @param pos 音符盒位置
     * @param instrument 乐器类型
     * @param note 音调
     */
    void _playNote(IWorld& world, const BlockPos& pos, i32 instrument, i32 note);
};

} // namespace blocks
} // namespace mc
