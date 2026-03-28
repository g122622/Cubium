#pragma once

#include "AbstractRailBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 探测铁轨方块
 *
 * 探测铁轨用于检测矿车：
 * - 矿车经过时输出红石信号
 * - 信号强度根据矿车内容物变化
 * - 只支持直轨和斜轨（不支持弯轨）
 *
 * 参考: net.minecraft.block.DetectorRailBlock
 */
class DetectorRailBlock : public AbstractRailBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DetectorRailBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 填充状态容器
     */
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    // ========== Tick ==========

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const override { return true; }

    /**
     * @brief 执行刻
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

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
     * @brief 获取强信号
     */
    [[nodiscard]] i32 getStrongPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side) const override;

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
};

} // namespace blocks
} // namespace mc
