#pragma once

#include "AbstractPressurePlateBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 测重压力板方块
 *
 * 测重压力板根据检测到的物品数量输出不同的信号强度。
 * - 轻质测重压力板：信号强度 = min(物品数量, 15)
 * - 重质测重压力板：信号强度 = min(物品数量 / 10, 15)
 *
 * 参考: net.minecraft.block.WeightedPressurePlateBlock
 */
class WeightedPressurePlateBlock : public AbstractPressurePlateBlock {
public:
    /**
     * @brief 测重压力板类型
     */
    enum class Sensitivity : u8 {
        Light = 0, ///< 轻质（每物品+1信号强度）
        Heavy = 1  ///< 重质（每10物品+1信号强度）
    };

    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param sensitivity 灵敏度类型
     */
    WeightedPressurePlateBlock(const BlockProperties& properties, Sensitivity sensitivity);

protected:
    [[nodiscard]] i32 calculateSignalStrength(IWorld& world, const BlockPos& pos) const override;

    [[nodiscard]] i32 getTickDelay(i32 oldSignal, i32 newSignal) const override;

    void playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const override;

private:
    Sensitivity m_sensitivity;

    /**
     * @brief 获取压力板上的实体数量
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return i32 实体数量
     */
    [[nodiscard]] i32 getEntityCount(IWorld& world, const BlockPos& pos) const;
};

} // namespace blocks
} // namespace mc
