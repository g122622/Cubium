#pragma once

#include "AbstractPressurePlateBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 木压力板方块
 *
 * 木压力板可以被所有实体触发（玩家、怪物、物品等）。
 * 只输出两种信号：0（无实体）和 15（有实体）。
 *
 * 参考: net.minecraft.block.WoodPressurePlateBlock
 */
class WoodPressurePlateBlock : public AbstractPressurePlateBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit WoodPressurePlateBlock(const BlockProperties& properties);

protected:
    [[nodiscard]] i32 calculateSignalStrength(IWorld& world, const BlockPos& pos) const override;

    [[nodiscard]] i32 getTickDelay(i32 oldSignal, i32 newSignal) const override;

    void playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const override;
};

} // namespace blocks
} // namespace mc
