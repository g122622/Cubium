#pragma once

#include "AbstractRailBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 普通铁轨方块
 *
 * 普通铁轨用于矿车行驶：
 * - 自动连接到相邻铁轨
 * - 支持弯轨和斜轨
 * - 无碰撞箱
 *
 * 参考: net.minecraft.block.RailBlock
 */
class RailBlock : public AbstractRailBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RailBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 填充状态容器
     */
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    // ========== 属性访问 ==========

    /**
     * @brief 获取铁轨形状
     */
    [[nodiscard]] RailShape getRailShape(const BlockState& state) const override;

    /**
     * @brief 设置铁轨形状
     */
    [[nodiscard]] BlockState withRailShape(const BlockState& state, RailShape shape) const override;

    /**
     * @brief 检查状态是否有铁轨形状属性
     */
    [[nodiscard]] bool hasRailShapeProperty(const BlockState& state) const override
    {
        return state.hasProperty(SHAPE());
    }

    /**
     * @brief 获取形状属性
     */
    static const EnumProperty<RailShape>& SHAPE()
    {
        static auto prop = RailShapeProperty::create("shape");
        return *prop;
    }
};

} // namespace blocks
} // namespace mc
