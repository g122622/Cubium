#pragma once

#include "../Block.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/property/Properties.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 方向性方块基类
 *
 * 支持6个方向（上下北南东西）的方块基类。
 * 提供旋转和镜像的默认实现。
 *
 * 参考: net.minecraft.block.DirectionalBlock
 */
class DirectionalBlock : public Block {
public:
    /**
     * @brief 获取 FACING 属性
     */
    static const DirectionProperty& FACING() {
        return BlockStateProperties::FACING();
    }

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DirectionalBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~DirectionalBlock() override = default;

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据玩家的朝向设置 FACING 属性。
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
     * @brief 获取朝向
     * @param state 方块状态
     * @return 朝向
     */
    [[nodiscard]] Direction getFacing(const BlockState& state) const;

    /**
     * @brief 设置朝向
     * @param state 方块状态
     * @param facing 朝向
     * @return 新状态
     */
    [[nodiscard]] const BlockState& withFacing(const BlockState& state, Direction facing) const;
};

} // namespace blocks
} // namespace mc
