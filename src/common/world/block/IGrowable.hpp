#pragma once

#include "../../core/Types.hpp"

namespace mc {

// Forward declarations
class IBlockReader;
class IWorld;
class BlockPos;
class BlockState;
namespace math {
class IRandom;
}

/**
 * @brief 可生长接口
 *
 * 实现此接口的方块可以被骨粉催熟。
 * 包括农作物、树苗、花朵、草等植物。
 *
 * 参考: net.minecraft.block.IGrowable
 */
class IGrowable {
public:
    virtual ~IGrowable() = default;

    /**
     * @brief 检查是否可以生长
     *
     * @param world 世界读取器
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param isClientSide 是否为客户端
     * @return 如果可以生长返回true
     */
    [[nodiscard]] virtual bool canGrow(
        IBlockReader& world,
        const BlockPos& pos,
        const BlockState& state,
        bool isClientSide) const = 0;

    /**
     * @brief 检查是否可以使用骨粉
     *
     * 对于某些方块（如草、花），骨粉有概率成功；
     * 对于农作物，骨粉必定有效（如果未成熟）。
     *
     * @param world 世界
     * @param random 随机数生成器
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return 如果骨粉有效返回true
     */
    [[nodiscard]] virtual bool canUseBonemeal(
        IWorld& world,
        math::IRandom& random,
        const BlockPos& pos,
        const BlockState& state) const = 0;

    /**
     * @brief 使用骨粉生长
     *
     * 当玩家对方块使用骨粉时调用。
     * 应该在服务端调用，因为会修改世界状态。
     *
     * @param world 世界
     * @param random 随机数生成器
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    virtual void grow(
        IWorld& world,
        math::IRandom& random,
        const BlockPos& pos,
        const BlockState& state) = 0;
};

} // namespace mc
