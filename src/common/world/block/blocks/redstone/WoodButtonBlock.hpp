#pragma once

#include "AbstractButtonBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 木按钮方块
 *
 * 木按钮被按下后持续 15 tick（1.5秒），比石头按钮长。
 * 木按钮还可以被箭触发。
 *
 * 参考: net.minecraft.block.WoodButtonBlock
 */
class WoodButtonBlock : public AbstractButtonBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit WoodButtonBlock(const BlockProperties& properties);

protected:
    void playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const override;
};

} // namespace blocks
} // namespace mc
