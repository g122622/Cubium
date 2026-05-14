#pragma once

#include "../../core/Types.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;
namespace fluid {
class Fluid;
}

/**
 * @brief 桶拾取处理器接口
 *
 * 实现此接口的方块可以被空桶从中取出流体。
 * 参考 MC 1.16.5 IBucketPickupHandler
 *
 * 当玩家使用空桶右键方块时，会调用 pickupFluid 方法。
 * 如果返回非空流体，则空桶变为对应流体的桶。
 */
class IBucketPickupHandler {
public:
    virtual ~IBucketPickupHandler() = default;

    /**
     * @brief 从方块中取出流体
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return 被取出的流体，如果无法取出则返回 nullptr
     */
    [[nodiscard]] virtual fluid::Fluid* pickupFluid(IWorld& world, const BlockPos& pos, const BlockState& state) = 0;
};

} // namespace mc
