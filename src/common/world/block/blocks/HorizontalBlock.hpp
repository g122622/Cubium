#pragma once

#include "../../../util/Direction.hpp"
#include "../../../util/property/Properties.hpp"
#include "../Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 水平方向方块基类
 *
 * 只支持4个水平方向（北南东西）的方块基类。
 * 提供旋转和镜像的默认实现。
 * 常用于门、床、活塞、熔炉等方块。
 *
 * 参考: net.minecraft.block.HorizontalBlock
 */
class HorizontalBlock : public Block {
public:
    /**
     * @brief 获取 HORIZONTAL_FACING 属性
     */
    static const DirectionProperty& FACING() { return BlockStateProperties::HORIZONTAL_FACING(); }

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit HorizontalBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~HorizontalBlock() override = default;

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据玩家的水平朝向设置 FACING 属性。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 旋转方块状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

protected:
    /**
     * @brief 获取水平朝向
     * @param state 方块状态
     * @return 朝向（仅水平方向）
     */
    [[nodiscard]] Direction getFacing(const BlockState& state) const;

    /**
     * @brief 设置水平朝向
     * @param state 方块状态
     * @param facing 朝向（仅水平方向）
     * @return 新状态
     */
    [[nodiscard]] const BlockState& withFacing(const BlockState& state, Direction facing) const;
};

} // namespace blocks
} // namespace mc
