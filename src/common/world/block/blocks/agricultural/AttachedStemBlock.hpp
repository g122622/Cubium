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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/PlantType.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/blocks/agricultural/StemGrownBlock.hpp"
#include "common/world/block/core/Block.hpp"

#include <unordered_map>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 连接茎方块
 *
 * 果实生成后茎变成的方块，朝向果实方向。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向（指向果实方向）
 *
 */
class AttachedStemBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param crop 对应的果实方块
     * @param properties 方块属性
     */
    AttachedStemBlock(const StemGrownBlock* crop, const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~AttachedStemBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 邻居更新 ==========

    /**
     * @brief 邻居方块更新处理
     *
     * 当果实被破坏时，将连接茎变回普通茎（AGE=7）。
     *
     * @param state 当前方块状态
     * @param facing 更新的方向
     * @param facingState 邻居状态
     * @param world 世界
     * @param currentPos 当前方块位置
     * @param facingPos 邻居位置
     * @return 更新后的状态
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     *
     * 形状根据朝向不同而不同，茎从中心延伸到果实方向。
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 物品 ==========

    /**
     * @brief 获取种子物品ID
     */
    [[nodiscard]] virtual u32 getSeedItem() const = 0;

    /**
     * @brief 获取对应的果实方块
     */
    [[nodiscard]] const StemGrownBlock* getCrop() const
    {
        return m_crop;
    }

    // ========== IPlantable 接口 ==========

    /**
     * @brief 获取植物类型 - 连接茎返回 PlantType::Crop
     */
    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;

protected:
    /// 对应的果实方块
    const StemGrownBlock* m_crop;

    /// 各方向的形状缓存（使用 unordered_map 存储）
    std::unordered_map<Direction, CollisionShape> m_shapesByDirection;
};

} // namespace blocks
} // namespace mc
