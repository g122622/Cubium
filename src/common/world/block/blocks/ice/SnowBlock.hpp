#pragma once

#include "../../Block.hpp"
#include "../../../../util/property/StateContainer.hpp"
#include "../../../../util/property/Properties.hpp"

#include <unordered_map>

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
 * @brief 雪层方块
 *
 * 可堆叠的雪层方块（1-8层），在光照足够时会融化。
 * 每层高度为 2 像素（1/8 方块）。
 *
 * MC ID: minecraft:snow
 *
 * 参考 MC 1.16.5 SnowBlock
 */
class SnowBlock : public Block {
public:
    /**
     * @brief 获取 LAYERS 属性
     */
    [[nodiscard]] static const IntegerProperty& LAYERS() {
        return BlockStateProperties::LAYERS_1_8();
    }

    /**
     * @brief 构造雪层方块
     */
    explicit SnowBlock(BlockProperties properties);

    /**
     * @brief 随机刻
     *
     * 在光照 > 11 时融化（掉落雪层物品并移除方块）。
     */
    void randomTick(
        IWorld& world,
        const BlockPos& pos,
        BlockState& state,
        math::IRandom& random
    ) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const override { return true; }
};

} // namespace blocks
} // namespace mc
