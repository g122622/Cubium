#pragma once

#include "../../Block.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;

namespace blocks {

/**
 * @brief 干海带块
 *
 * 由9个干海带合成的方块，可以作为燃料使用。
 * 可以分解为9个干海带物品。
 *
 * MC ID: minecraft:dried_kelp_block
 *
 * 参考 MC 1.16.5 DriedKelpBlock
 */
class DriedKelpBlock : public Block {
public:
    /**
     * @brief 构造干海带块
     */
    explicit DriedKelpBlock(BlockProperties properties);

    /**
     * @brief 获取燃烧时间
     * 干海带块可以作为燃料，燃烧时间为200tick（10秒）
     * @return 燃烧时间（tick），0表示不可燃
     */
    [[nodiscard]] i32 getBurnTime() const { return 200; }

    /**
     * @brief 获取易燃性
     * @return 易燃等级（0-100，越高越易燃）
     */
    [[nodiscard]] i32 getFlammability() const { return 60; }
};

} // namespace blocks
} // namespace mc
