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

#include "world/block/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief TNT方块
 *
 * TNT是一种可以被红石信号或火焰点燃的爆炸性方块。
 *
 * ## 特性
 * - 红石触发点燃
 * - 火焰/熔岩点燃
 * - 被箭矢点燃
 * - 爆炸产生伤害和破坏
 * - 爆炸后掉落物品（如果未点燃）
 *
 * 参考: net.minecraft.block.TNTBlock
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

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Normal;
    }

    // ========== TNT特有方法 ==========

    /**
     * @brief 检查TNT是否不稳定（即将爆炸）
     * @param state 方块状态
     * @return true 如果不稳定
     */
    [[nodiscard]] static bool isUnstable(const BlockState& state);

    /**
     * @brief 点燃TNT
     *
     * @param world 世界引用
     * @param pos TNT位置
     * @param source 点燃来源（可为nullptr）
     */
    void ignite(IWorld& world, const BlockPos& pos, const BlockState& state);

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
    [[nodiscard]] bool hasFlammableNeighbor(IWorld& world, const BlockPos& pos) const;
};

} // namespace blocks
} // namespace mc
