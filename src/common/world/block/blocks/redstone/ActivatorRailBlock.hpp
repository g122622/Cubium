#pragma once

#include "AbstractRailBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 激活铁轨方块
 *
 * 激活铁轨用于激活矿车：
 * - 接收红石信号时激活
 * - 激活时可以：
 *   - 弹出矿车中的实体（运输矿车）
 *   - 切换矿车轨道（TNT矿车、漏斗矿车）
 * - 只支持直轨和斜轨（不支持弯轨）
 *
 * 参考: net.minecraft.block.ActivatorRailBlock
 */
class ActivatorRailBlock : public AbstractRailBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ActivatorRailBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 填充状态容器
     */
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    // ========== 红石 ==========

    /**
     * @brief 邻居更新
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

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
     * @brief 是否被激活
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 获取形状属性
     */
    static const EnumProperty<RailShape>& SHAPE()
    {
        static auto prop = RailShapeProperty::create("shape");
        return *prop;
    }

    /**
     * @brief 获取激活属性
     */
    static const BooleanProperty& POWERED() { return BlockStateProperties::POWERED(); }
};

} // namespace blocks
} // namespace mc
