#pragma once

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
 * @brief 冰方块
 *
 * 透明冰方块，在明亮环境中会融化成水。
 * 挖掘后会变成水源方块。
 *
 * MC ID: minecraft:ice
 *
 * 参考 MC 1.16.5 IceBlock
 */
class IceBlock : public Block {
public:
    /**
     * @brief 构造冰方块
     */
    explicit IceBlock(BlockProperties properties);

    /**
     * @brief 方块被移除后
     * 冰在非寒冷生物群系会融化成水，在温暖光源附近也会融化
     */
    void onBlockRemoved(
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state
    ) override;

    /**
     * @brief 随机刻
     * 在明亮环境中融化
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

/**
 * @brief 浮冰方块
 *
 * 不透明的冰方块，不会融化。
 * 挖掘后会掉落自身（使用精准采集）或什么都不掉落。
 *
 * MC ID: minecraft:packed_ice
 *
 * 参考 MC 1.16.5 PackedIceBlock
 */
class PackedIceBlock : public Block {
public:
    /**
     * @brief 构造浮冰方块
     */
    explicit PackedIceBlock(BlockProperties properties);
};

/**
 * @brief 蓝冰方块
 *
 * 最光滑的冰方块，摩擦力极低。
 * 可以用浮冰合成，不会融化。
 *
 * MC ID: minecraft:blue_ice
 *
 * 参考 MC 1.16.5 BlueIceBlock
 */
class BlueIceBlock : public Block {
public:
    /**
     * @brief 构造蓝冰方块
     */
    explicit BlueIceBlock(BlockProperties properties);
};

/**
 * @brief 霜冰方块
 *
 * 由冰霜行者附魔生成的临时冰方块。
 * 在光源附近会融化成水。
 *
 * MC ID: minecraft:frosted_ice
 *
 * 参考 MC 1.16.5 FrostedIceBlock
 */
class FrostedIceBlock : public Block {
public:
    /**
     * @brief 构造霜冰方块
     */
    explicit FrostedIceBlock(BlockProperties properties);

    /**
     * @brief 随机刻
     * 在光源附近融化
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
