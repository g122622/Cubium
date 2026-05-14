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

#include "../../../../util/Direction.hpp"
#include "../../../../util/property/EnumProperty.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"

namespace mc {

namespace util {
template <typename T>
class EnumProperty;
}

namespace blocks {

/**
 * @brief 活塞头方块
 *
 * 活塞头是活塞伸出时显示的方块部分。
 *
 * ## 特性
 * - 仅在活塞伸出时显示
 * - 与活塞主体关联
 * - 推动其他方块
 *
 * 参考: net.minecraft.block.PistonHeadBlock
 */
class PistonHeadBlock : public Block {
public:
    /**
     * @brief 活塞头类型
     */
    enum class Type : u8 {
        Normal = 0, ///< 普通活塞头
        Sticky = 1  ///< 粘性活塞头
    };

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit PistonHeadBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Block;
    }

    // ========== 活塞头特有方法 ==========

    /**
     * @brief 获取活塞头朝向
     *
     * @param state 方块状态
     * @return Direction 朝向方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 获取活塞头类型
     *
     * @param state 方块状态
     * @return Type 活塞头类型
     */
    [[nodiscard]] static Type getType(const BlockState& state);

    /**
     * @brief 设置活塞头类型
     *
     * @param state 方块状态
     * @param type 类型
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withType(BlockState state, Type type);

    /**
     * @brief 获取活塞头类型属性
     *
     * 用于 MovingPistonBlock 共享相同的属性。
     *
     * @return 类型属性的引用
     */
    [[nodiscard]] static const EnumProperty<Type>& getTypeProperty();
};

} // namespace blocks
} // namespace mc
