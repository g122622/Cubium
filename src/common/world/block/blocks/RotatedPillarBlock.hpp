#pragma once

#include "../../../util/Direction.hpp"
#include "../../../util/property/DirectionProperty.hpp"
#include "../Block.hpp"

namespace mc {

// Forward declarations
class BlockState;

/**
 * @brief 旋转柱状方块基类
 *
 * 用于原木、柱状玄武岩等可绕Y轴旋转的方块。
 * 拥有 axis 属性（X、Y、Z）。
 *
 * 参考: net.minecraft.block.RotatedPillarBlock
 */
class RotatedPillarBlock : public Block {
public:
    /**
     * @brief 获取AXIS属性
     */
    static const EnumProperty<Axis>& AXIS();

    /**
     * @brief 构造函数
     */
    RotatedPillarBlock(BlockProperties properties);

    /**
     * @brief 获取方块的轴
     */
    Axis getAxis(const BlockState& state) const;

    /**
     * @brief 设置方块的轴
     * @return 新状态
     */
    const BlockState& withAxis(const BlockState& state, Axis axis) const;

    /**
     * @brief 旋转方块状态
     *
     * 当结构旋转时，X轴和Z轴会互换。
     *
     * @param state 原状态
     * @param rotation 旋转类型
     * @return 旋转后的状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据放置面的轴向设置初始状态。
     *
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;
};

} // namespace mc
