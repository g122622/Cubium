#pragma once

#include "AbstractButtonBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 石头按钮方块
 *
 * 石头按钮被按下后持续 10 tick（1 秒）。
 *
 * 参考: net.minecraft.block.StoneButtonBlock
 */
class StoneButtonBlock : public AbstractButtonBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit StoneButtonBlock(const BlockProperties& properties);

protected:
    void playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const override;
};

} // namespace blocks
} // namespace mc
