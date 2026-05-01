#pragma once

#include "../../Block.hpp"
#include "../../../../util/property/Properties.hpp"

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
 * @brief 可蔓延的雪覆盖泥土方块基类
 *
 * 这是草方块和菌丝的基类，提供蔓延和退化机制：
 * - 当光照不足时退化成泥土
 * - 当光照足够时向周围泥土蔓延
 *
 * 参考 MC 1.16.5 SpreadableSnowyDirtBlock
 */
class SpreadableSnowyDirtBlock : public Block {
public:
    explicit SpreadableSnowyDirtBlock(BlockProperties properties);

    /**
     * @brief 随机刻
     *
     * 处理蔓延和退化逻辑：
     * - 光照不足时退化成泥土
     * - 光照充足时向周围泥土蔓延
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

protected:
    /**
     * @brief 检查是否为雪覆盖条件
     *
     * 检查上方是否有雪或者光照条件是否满足。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果满足蔓延条件
     */
    [[nodiscard]] static bool isSnowyConditions(
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state
    );

    /**
     * @brief 检查是否为雪覆盖且非水下条件
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果满足蔓延条件且不在水下
     */
    [[nodiscard]] static bool isSnowyAndNotUnderwater(
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state
    );
};

/**
 * @brief 草方块
 *
 * 可蔓延的草方块，在光照充足时向周围泥土蔓延，
 * 在光照不足时退化成泥土。
 *
 * MC ID: minecraft:grass_block
 *
 * 参考 MC 1.16.5 GrassBlock
 */
class GrassBlock : public SpreadableSnowyDirtBlock {
public:
    explicit GrassBlock(BlockProperties properties);
};

/**
 * @brief 菌丝方块
 *
 * 可蔓延的菌丝方块，在光照充足时向周围泥土蔓延，
 * 在光照不足时退化成泥土。
 *
 * MC ID: minecraft:mycelium
 *
 * 参考 MC 1.16.5 MyceliumBlock
 */
class MyceliumBlock : public SpreadableSnowyDirtBlock {
public:
    explicit MyceliumBlock(BlockProperties properties);
};

} // namespace blocks
} // namespace mc
