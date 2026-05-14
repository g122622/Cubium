#pragma once

#include "../../../../util/property/Properties.hpp"
#include "../agricultural/BushBlock.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 双格植物方块基类
 *
 * 用于高度为2格的植物（高草、大型蕨、向日葵等）。
 * 上半部分和下半部分使用同一个方块，通过 HALF 属性区分。
 *
 * 状态属性：
 * - HALF: DoubleBlockHalf (UPPER, LOWER)
 *
 * 参考: net.minecraft.block.DoublePlantBlock
 */
class DoublePlantBlock : public BushBlock {
public:
    // 使用 BlockStateProperties 中的 DoubleBlockHalf 枚举
    using DoubleBlockHalf = BlockStateProperties::DoubleBlockHalf;

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DoublePlantBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~DoublePlantBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取半部分属性
     */
    static const EnumProperty<DoubleBlockHalf>& halfProperty();

    /**
     * @brief 获取方块的半部分
     */
    [[nodiscard]] DoubleBlockHalf getHalf(const BlockState& state) const;

    /**
     * @brief 创建指定半部分的状态
     */
    [[nodiscard]] BlockState withHalf(DoubleBlockHalf half) const;

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查是否可以放置
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居更新
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 其他 ==========

    /**
     * @brief 放置上半部分
     * @param world 世界
     * @param pos 位置（下半部分位置）
     * @param state 下半部分状态
     * @param flags 更新标志
     * @return 是否成功放置
     */
    static bool placeAt(IWorld& world, const BlockPos& pos, const BlockState& state, i32 flags);

protected:
    /// 下半部分形状
    CollisionShape m_lowerShape;
    /// 上半部分形状
    CollisionShape m_upperShape;
};

} // namespace blocks
} // namespace mc
