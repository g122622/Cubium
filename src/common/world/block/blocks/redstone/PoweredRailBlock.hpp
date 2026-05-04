#pragma once

#include "AbstractRailBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 动力铁轨方块
 *
 * 动力铁轨用于加速矿车：
 * - 接收红石信号时激活
 * - 激活时加速矿车
 * - 未激活时减速矿车
 * - 只支持直轨和斜轨（不支持弯轨）
 *
 * 参考: net.minecraft.block.PoweredRailBlock
 */
class PoweredRailBlock : public AbstractRailBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit PoweredRailBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 填充状态容器
     */
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    // ========== 红石 ==========

    /**
     * @brief 获取弱信号
     */
    [[nodiscard]] i32 getWeakPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side) const override;

    /**
     * @brief 邻居更新
     */
    void neighborChanged(
        IWorld& world,
        const BlockPos& pos,
        Block& neighborBlock,
        const BlockPos& neighborPos,
        bool isMoving) override;

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
    [[nodiscard]] bool hasRailShapeProperty(const BlockState& state) const override {
        return state.hasProperty(SHAPE());
    }

    /**
     * @brief 是否被激活
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 获取形状属性
     */
    static const EnumProperty<RailShape>& SHAPE() {
        static auto prop = RailShapeProperty::create("shape");
        return *prop;
    }

    /**
     * @brief 获取激活属性
     */
    static const BooleanProperty& POWERED() {
        return BlockStateProperties::POWERED();
    }

private:
    /**
     * @brief 沿铁轨方向查找动力信号
     *
     * MC 1.16.5: findPoweredRailSignal
     * 使用迭代方式搜索相连的动力铁轨，最大搜索距离为8格。
     *
     * @param world 世界
     * @param startPos 起始铁轨位置
     * @param startState 起始铁轨状态
     * @param checkForward 是否向前检查（true为正向，false为反向）
     * @return 如果找到充能的动力铁轨则返回true
     */
    [[nodiscard]] bool findPoweredRailSignal(
        IWorld& world,
        const BlockPos& startPos,
        const BlockState& startState,
        bool checkForward) const;
};

} // namespace blocks
} // namespace mc
