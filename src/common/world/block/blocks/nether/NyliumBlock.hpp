#pragma once

#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 绯红/诡异菌岩方块
 *
 * 下界的可蔓延岩类方块，在光照过高时会退化为下界岩。
 *
 * MC ID: minecraft:crimson_nylium, minecraft:warped_nylium
 *
 * 参考 MC 1.16.5 NyliumBlock
 */
class NyliumBlock : public Block {
public:
    explicit NyliumBlock(BlockProperties properties);

    /**
     * @brief 随机刻
     *
     * 在光照过亮时退化为下界岩。
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const override { return true; }

private:
    /**
     * @brief 检查位置是否足够暗
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @return true 如果足够暗（不会退化）
     */
    [[nodiscard]] static bool isDarkEnough(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
