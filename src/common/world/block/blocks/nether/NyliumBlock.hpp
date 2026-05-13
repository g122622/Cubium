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

private:
    /**
     * @brief 检查位置是否足够暗
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @return true 如果足够暗（不会退化）
     */
    [[nodiscard]] static bool isDarkEnough(
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state
    );
};

/**
 * @brief 岩浆块方块
 * TODO 移到单独文件中，很简单
 *
 * 下界的岩浆块，站在上面会受伤，在水中会产生气泡柱。
 *
 * MC ID: minecraft:magma_block
 *
 * 参考 MC 1.16.5 MagmaBlock
 */
class MagmaBlock : public Block {
public:
    explicit MagmaBlock(BlockProperties properties);

    /**
     * @brief 方块被添加时
     *
     * 调度 tick 以检查气泡柱。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居更新
     *
     * 当上方有水时调度 tick。
     */
    void neighborChanged(
        IWorld& world,
        const BlockPos& pos,
        Block& neighborBlock,
        const BlockPos& neighborPos,
        bool isMoving
    ) override;

    /**
     * @brief Tick 更新
     *
     * 在上方生成气泡柱。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 随机刻
     *
     * 在水中产生气泡效果。
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
